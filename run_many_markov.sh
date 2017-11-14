#!/bin/bash

set -eu -o pipefail

jobName=(mar_1 mar_2 mar_3 mar_4)
nblocks=(128 640 6400)

function writeJob {
    pis=$4
    rfs=$1
    efs=$2
    nblocks=$3
    enb=$5
    jn=$6
    jobN=$jn.$nblocks.job
    echo -e "#!/bin/bash" > ./$jobN
    echo -e "#$ -cwd -m ea"  >> ./$jobN

    for pib in ${pis[*]}
    do
        for rf in ${rfs[*]}
        do
            for ef in ${efs[*]}
            do
                printf "python3 run_uep_markov.py --overhead 0.3 %s %s %s %s %s\n" $rf $ef $nblocks $pib $enb >> ./$jobN
                #python3 run_uep_iid.py $rf $ef $nblocks
            done
        done
    done
    qsub $jobN
}

for nbl in ${nblocks[*]}
do
    for jn in ${jobName[*]}
    do
        if [ $jn=mar_1 ]; then
            rfs=(1 3 5)
            efs=(1)
            pib=(inf)
            enb=(1)
            writeJob $rfs $efs $nbl $pib $enb $jn
        else
            if [ $jn=mar_2 ]; then
                rfs=(1 3 5)
                efs=(1)
                pib=(0.01 0.1)
                enb=(5)
                writeJob $rfs $efs $nbl $pib $enb $jn
            else
                if [ $jn=mar_3 ]; then
                    rfs=(1 3 5)
                    efs=(1)
                    pib=(0.01 0.1)
                    enb=(10)
                    writeJob $rfs $efs $nbl $pib $enb $jn
                else
                    rfs=(1 3 5)
                    efs=(1)
                    pib=(0.01 0.1)
                    enb=(50)
                    writeJob $rfs $efs $nbl $pib $enb $jn
                fi
            fi
        fi
    done
done