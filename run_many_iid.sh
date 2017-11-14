#!/bin/bash

set -eu -o pipefail

rfs=(1 3 4 5)
efs=(1 2)
#rfs=(3)
#efs=(4)
nblocks=640
pers=(0 0.1)
echo -e "#!/bin/bash" > ./run_uep_iid.job
echo -e "#$ -cwd -m ea"  >> ./run_uep_iid.job

for per in ${pers[*]}
do
    for rf in ${rfs[*]}
    do
        for ef in ${efs[*]}
        do
            printf "python3 run_uep_iid.py --iid_per  %s %s %s %s\n" $per $rf $ef $nblocks >> ./run_uep_iid.job
            #python3 run_uep_iid.py $rf $ef $nblocks
        done
    done
done
qsub run_uep_iid.job