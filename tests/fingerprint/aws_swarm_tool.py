#!/bin/env python3

import os
import re
import sys
import time
import subprocess

import boto3
from botocore.exceptions import ClientError

ec2 = boto3.resource("ec2")
cloudformation = boto3.resource("cloudformation")
autoscaling = boto3.client("autoscaling")


def importKeyPair(filename, keypair_name):
    """ imports an existing local key into aws """
    pass


def create_add_key_pair(key_pair_name):
    key_pair = ec2.create_key_pair(KeyName=key_pair_name)

    old_umask = os.umask(0o177)
    try:
        with open(key_pair_name + ".pem", "wt", 0o600) as key_file:
            key_file.write(key_pair.key_material)
    except IOError:
        pass
    finally:
        os.umask(old_umask)

    subprocess.call(["ssh-add", key_pair_name + ".pem"])


def createSwarm(stack_name, key_pair_name, num_workers=3):
    """ creates the various resources on AWS using the official "Docker for AWS" CloudFormation Template """

    stack = cloudformation.create_stack(
        StackName=stack_name,
        TemplateURL='https://editions-us-east-1.s3.amazonaws.com/aws/stable/Docker.tmpl',
        Capabilities=['CAPABILITY_IAM'],
        Parameters=[
            {'ParameterKey': 'KeyName',             'ParameterValue': key_pair_name},
            {'ParameterKey': 'InstanceType',        'ParameterValue': "c4.large"},
            {'ParameterKey': 'ManagerInstanceType', 'ParameterValue': "c4.large"},
            {'ParameterKey': 'ManagerSize',         'ParameterValue': '1'},
            {'ParameterKey': 'ClusterSize',         'ParameterValue': str(num_workers)},
        ])

    print("Waiting for resources to be created... This will take about 10 minutes, and create about 42 resources.")
    while stack.stack_status != 'CREATE_COMPLETE':
        time.sleep(5)

        stack.reload()
        resources = list(stack.resource_summaries.iterator())

        num_ready = sum(
            1 for r in resources if r.resource_status == 'CREATE_COMPLETE')
        num_creating = sum(
            1 for r in resources if r.resource_status == 'CREATE_IN_PROGRESS')

        sys.stdout.write(u"\u001b[1000D\u001b[0K")

        print("|" + num_ready * "=" + num_creating * "+" +
              (42 - num_creating - num_ready) * " " + "|", end="", flush=True)

    print()
    print("Done.")

    # create a cloudwatch alarm for low cpu usage, then a scaling policy to shutdown the machines


def connectToSwarm(stack_name):
    """ creates an SSH tunnel to the manager machine, forwarding all necessary ports """

    stack = cloudformation.Stack(stack_name)

    output_managers = [
        o for o in stack.outputs if o['OutputKey'] == 'Managers']
    output_managers_value = output_managers[0]['OutputValue']

    manager_asg_name = re.findall(
        r"groupName=([-a-zA-Z0-9]+)", output_managers_value)[0]
    group_info = autoscaling.describe_auto_scaling_groups(
        AutoScalingGroupNames=[manager_asg_name])

    manager_instance_id = group_info['AutoScalingGroups'][0]['Instances'][0]['InstanceId']
    manager_instance = ec2.Instance(manager_instance_id)
    manager_public_ip = manager_instance.public_ip_address

    subprocess.call([
        "ssh", "docker@" + manager_public_ip, "-N", "-o", "StrictHostKeyChecking=no",
        "-L", "localhost:2374:/var/run/docker.sock", "-L", "9181:172.17.0.1:9181",
        "-L", "8080:172.17.0.1:8080", "-L", "27017:172.17.0.1:27017", "-L", "6379:172.17.0.1:6379"])

    # from now on:  docker -H tcp://localhost:2374 info

    print("DONE")


def deployApp():
    subprocess.call(["docker", "-H", "tcp://localhost:2374", "stack", "up",
                     "--compose-file", "docker-compose-fingerprints.yml", "inet"])


def haltSwarm():
    pass


def continueSwarm():
    pass


def removeSwarm(stack_name):
    stack = cloudformation.Stack(stack_name)

    stack.delete()
    print("Waiting for resources to be deleted... This will take a couple minutes.")

    while stack.stack_status == 'DELETE_IN_PROGRESS':
        time.sleep(5)

        try:
            stack.reload()
            resources = list(stack.resource_summaries.iterator())
        except ClientError:
            break

        num_deleting = sum(
            1 for r in resources if r.resource_status == 'DELETE_IN_PROGRESS')
        num_remaining = sum(
            1 for r in resources if r.resource_status == 'CREATE_COMPLETE')

        sys.stdout.write(u"\u001b[1000D\u001b[0K")

        print("|" + num_remaining * "=" + num_deleting * "-" +
              (42 - num_remaining - num_deleting) * " " + "|", end="", flush=True)

    print()
    print("Done.")

    pass
