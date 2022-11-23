#!/bin/env python3

# Creates HTCondor jobs for seedRebuild_cfg.py

import os
import stat
from subprocess import run, PIPE, CalledProcessError
from argparse import ArgumentParser

datasets = {
    'ZToMuMu_M-50To120': '/ZToMuMu_M-50To120_TuneCP5_14TeV-powheg-pythia8/Run3Summer21DRPremix-120X_mcRun3_2021_realistic_v6-v2/GEN-SIM-RECO',
    'JPsiToMuMu'       : '/JPsiToMuMu_Pt-0To100-pythia8-gun/Run3Summer21DRPremix-120X_mcRun3_2021_realistic_v6-v2/GEN-SIM-RECO' 
}


def condorSubScript( mainDir, jobFlavour='espresso' ):
    '''prepare the Condor submission script'''
    script = '''\
executable              = $(directory)/batchScript.sh
arguments               = {mainDir}/$(directory) $(ClusterId)$(ProcId)
output                  = log/$(ClusterId).$(ProcId).out
error                   = log/$(ClusterId).$(ProcId).err
log                     = log/$(ClusterId).log
Initialdir              = $(directory)
request_memory          = 4000M
#Possible values: https://batchdocs.web.cern.ch/local/submit.html
+JobFlavour             = "{jobFlavour}"

x509userproxy           = {home}/x509up_u{uid}

#https://www-auth.cs.wisc.edu/lists/htcondor-users/2010-September/msg00009.shtml
periodic_remove         = JobStatus == 5

ShouldTransferFiles     = YES
'''
    return script.format(home=os.path.expanduser("~"), uid=os.getuid(), mainDir=mainDir, jobFlavour=jobFlavour)


batchScript = '''#!/bin/bash
set -euox pipefail

if [ -z ${_CONDOR_SCRATCH_DIR+x} ]; then
  #running locally
  runninglocally=true
  _CONDOR_SCRATCH_DIR=$(mktemp -d)
  SUBMIT_DIR=$(pwd)
else
  runninglocally=false
  SUBMIT_DIR=$1
fi

cd $SUBMIT_DIR
eval $(scram ru -sh)

cp run_cfg.py $_CONDOR_SCRATCH_DIR
cd $_CONDOR_SCRATCH_DIR

echo 'Running at:' $(date)
echo path: `pwd`

cmsRunStatus=   #default for successful completion is an empty file
cmsRun run_cfg.py |& grep -v -e 'MINUIT WARNING' -e 'Second derivative zero' -e 'Negative diagonal element' -e 'added to diagonal of error matrix' > log.txt || cmsRunStatus=$?

echo -n $cmsRunStatus > exitStatus.txt
echo 'cmsRun done at: ' $(date) with exit status: ${cmsRunStatus+0}
gzip log.txt

export ROOT_HIST=0

echo "Files on node:"
ls -la

#delete mela stuff and $USER.cc
#I have no idea what $USER.cc is
rm -f br.sm1 br.sm2 ffwarn.dat input.DAT process.DAT "$USER.cc"

#delete submission scripts, so that they are not copied back (which fails sometimes)
rm -f run_cfg.py batchScript.sh

echo '...done at' $(date)

#note cping back is handled automatically by condor
if $runninglocally; then
  cp *.root *.txt *.gz $SUBMIT_DIR
  [ -e *.log ] && cp *log $SUBMIT_DIR
fi

exit $cmsRunStatus
''' 

parser = ArgumentParser(
    description = 'Create folders with scripts and configs to submit to HTCondor'
)
parser.add_argument('-d', '--dataset'   , choices=[*datasets.keys()], default='ZToMuMu_M-50To120')
parser.add_argument('-j', '--flavour'   , choices=['espresso', 'microcentury', 'longlunch', 'workday', 'tomorrow', 'testmatch', 'nextweek'], default='longlunch')
parser.add_argument(      '--force'     , action='store_true', help='Delete any existing job folders')
parser.add_argument('-n', '--nMax'      , default=0, type=int, help='Number of files to use; 0 means all [default]')
parser.add_argument('-o', '--output_dir', default='production', help='Base directory for the jobs')
parser.add_argument('-e', '--events'    , default=-1, type=int, help='Maximum number of events processed per file')

args = parser.parse_args()

print('INFO: configuration =', args.__dict__)

# Get file list from DAS
try:
    dasProcess = run('dasgoclient -query "file dataset={}"'.format(datasets[args.dataset]), check=True, shell=True, encoding='ASCII', stdout=PIPE, stderr=PIPE)
except CalledProcessError as e:
    print(e.output)  # dasgoclient prints errors to stdout, for some reasons
    raise e
filesDAS = dasProcess.stdout.rstrip('\n').split('\n')

# If requested, use only the first nMax files
if(args.nMax > 0):
    files = filesDAS[:args.nMax]
else:
    files = filesDAS
print('INFO: using {:d} files (out of {:d})'.format(len(files), len(filesDAS)))

os.makedirs(args.output_dir, exist_ok=True)

jobDirectory = os.path.join(args.output_dir, args.dataset)
try:
    os.mkdir(jobDirectory)
except FileExistsError as e:
    if(args.force):
        print('INFO: removing existing jobs in', jobDirectory)
        run('rm -r '+jobDirectory+'/*', shell=True, check=True)
    else:
        print('ERROR: Job folder exists. If you want to recreate it run agin with --force')
        raise e

# Create condor.sub
absJobDirectory = run(['realpath', jobDirectory], encoding='ASCII', stdout=PIPE).stdout.strip('\n')
with open(os.path.join(jobDirectory, 'condor.sub'), 'w') as condorsub:
    condorsub.write(condorSubScript( absJobDirectory, jobFlavour=args.flavour ))

# String to append to the run_cfg.py
str_to_append = '''
# createJobs.py: replacing fileNames and maxEvents
process.source.fileNames = cms.untracked.vstring('{fileName}')
process.maxEvents.input = cms.untracked.int32({events:d})\n
'''.format(events=args.events, fileName='{fileName}')


for i, fileName in enumerate(files):
    #Create job dir
    chunkDir = os.path.join(jobDirectory, 'Chunk_{:d}'.format(i))
    os.mkdir( chunkDir )
    
    # Create script and make it executable
    scriptPath = os.path.join(chunkDir, 'batchScript.sh')
    with open(scriptPath, 'w') as script:
        script.write(batchScript)
    os.chmod(scriptPath, os.stat(scriptPath).st_mode | stat.S_IEXEC)
    
    # Copy cfg.py replacing the filename 
    run(['cp', 'seedRebuild_cfg.py', '{}/run_cfg.py'.format(chunkDir)], check=True)
    with open('{}/run_cfg.py'.format(chunkDir), 'a') as cfg:
        cfg.write(str_to_append.format(fileName=fileName))
    
    # Create logdir
    os.makedirs(os.path.join(chunkDir, 'log'), exist_ok=True)

print("INFO: created {:d} jobs in {:s}".format(i+1, absJobDirectory))
