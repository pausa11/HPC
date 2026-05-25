aws cloudformation describe-stack-events --stack-name hpc-mpi-cluster --query "StackEvents[?ResourceStatus=='CREATE_FAILED'].[LogicalResourceId,ResourceStatusReason]" --output table


pcluster describe-cluster --cluster-name hpc-mpi-cluster --query 'headNode.publicIpAddress' --output text

"3.235.161.247"

scp -i ~/.ssh/hpc-mpi-key.pem -r \
  /Users/danieltorosoto/universidad/HPC/caso-estudio-3 \
  ec2-user@3.235.161.247:/shared/caso-estudio-3

scp -i ~/.ssh/hpc-mpi-key.pem \
    /Users/danieltorosoto/universidad/HPC/caso-estudio-3/scripts/RunAll.sh \
    ec2-user@3.235.161.247:/shared/caso-estudio-3/scripts/RunAll.sh

ssh -i ~/.ssh/hpc-mpi-key.pem ec2-user@3.235.161.247

salloc --nodes=4 --ntasks=8 --ntasks-per-node=2 --partition=compute