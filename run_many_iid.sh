#!/bin/bash

set -eu -o pipefail

rfs=(1 3 4 5)
efs=(1 2)
nblocks=10000
echo -e "#!/bin/bash" > ./run_uep_iid.job
echo -e "#$ -cwd -m ea"  >> ./run_uep_iid.job

for rf in ${rfs[*]}
do
    for ef in ${efs[*]}
    do
        printf "python3 run_uep_iid.py %s %s %s\n" $rf $ef $nblocks >> ./run_uep_iid.job
        #python3 run_uep_iid.py $rf $ef $nblocks
    done
done

qsub run_uep_iid.job