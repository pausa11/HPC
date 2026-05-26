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

salloc --nodes=3 --ntasks=6 --ntasks-per-node=2 --partition=compute

scp -i ~/.ssh/hpc-mpi-key.pem -r ec2-user@3.235.161.247:/shared/caso-estudio-3/stats /Users/danieltorosoto/universidad/HPC/caso-estudio-3/stats/

nohup bash RunAll.sh > runall.log 2>&1 &

vamos a crear el informe del caso de estudio 3 @caso-estudio-3/latex/HPCG2-CE03- DanielToroSoto-JuanCamiloGalvis.tex , es un .tex , toma como plantilla @caso- estudio-1/docs/informe.tex . para este caso de estudio usamos aws cli + parallelcluster (requeria python 3.9 por temas de compatibilidad). La configuracion del cluster @caso-estudio-3/hpc-cluster-config.yaml (nota, el headnode con t3 micro tiene 1gbram y 2vcpus. los nodos de computo c7i-flex.large tiene 2vcpus con 8gbram). los comando para interactuar con el headnode estan en @test.md . el plan guia que se ejecuto fue @caso-estudio-3/docs/plan.md (algunos comandos se cambiaron, como el de salloc, estos estan en test.md). toma la documentacion y apuntes de @caso-estudio-3/docs/ usala y que sea congruente con el desarrollo del informe. Tambien vamos a usar @caso-estudio-3/images que tenemos evidencias de ciertos momentos del desarrollo del proyecto. Los datos para las conclusiones estan en @caso-estudio-3/stats , ademas queremos graficas para hacer la interpretacion de los resultado mas sencilla, para esto te puedes basar de @caso-estudio-1/docs/charts , la mas importante es la grafica de speed up, el speed up base es el secuencial.