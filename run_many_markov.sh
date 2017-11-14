#!/bin/bash

set -eu -o pipefail

rfs=(1 3)
efs=(1 3)
#EnG=(960 999)
EnG=(960)
max_corr=1000
nblocks=640

echo -e "#!/bin/bash" > ./run_uep_markov.job
echo -e "#$ -cwd -m ea"  >> ./run_uep_markov.job

for eng in ${EnG[*]}
do
    let "enb = $max_corr - $eng"
    for rf in ${rfs[*]}
    do
        for ef in ${efs[*]}
        do
            printf "python3 run_uep_markov.py %s %s %s %s %s\n" $rf $ef $nblocks $eng $enb >> ./run_uep_markov.job
            #python3 run_uep_iid.py $rf $ef $nblocks
        done
    done
done
#qsub run_uep_markov.job
