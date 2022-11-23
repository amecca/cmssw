#!/bin/sh

# Checks the status of the chunks/jobs in the current folder and moves the succesful ones to $gooddir

gooddir=AAAOK

get_cause() {
    case $1 in
	-1)  echo "Error return without specification" ;;
	1)   echo "Hangup (POSIX)" ;;
	2)   echo "Interrupt (ANSI)" ;;
	4)   echo "Illegal instruction (ANSI)" ;;
	5)   echo "Trace trap (POSIX)" ;;
	6)   echo "Abort (ANSI) or IOT trap (4.2BSD)" ;;
	7)   echo "BUS error (4.2BSD)" ;;
	8)   echo "Floating point exception (ANSI)" ;;
	9)   echo "killed, unblockable (POSIX) kill -9" ;;
	11)  echo "segmentation violation (ANSI) (most likely user application crashed)" ;;
	15)  echo "Termination (ANSI)" ;;
	24)  echo "Soft CPU limit exceeded (4.2 BSD)" ;;
	25)  echo "File size limit exceeded (4.2 BSD)" ;;
	30)  echo "Power failure restart (System V.)" ;;
	50)  echo "Required application version not found at the site." ;;
	64)  echo "I/O error: cannot open data file (SEAL)" ;;
	65)  echo "End of job from user application (CMSSW)" ;;
	66)  echo "Application exception" ;;
	73)  echo "Failed writing to read-only file system" ;;
	81)  echo "Job did not find functioning CMSSW on worker node." ;;
	84)  echo "Some required file not found; check logs for name of missing file." ;;
	85)  echo "Error reading file" ;;
	86)  echo "Fatal ROOT error (probably while reading file)" ;;
	90)  echo "Application exception" ;;
	92)  echo "Failed to open file" ;;
	126) echo "Permission problem or command is not an executable" ;;
	127) echo "Command not found" ;;
	129) echo "Hangup (POSIX)" ;;
	132) echo "Illegal instruction (ANSI)" ;;
	133) echo "Trace trap (POSIX)" ;;
	134) echo "Abort (ANSI) or IOT trap (4.2 BSD) (most likely user application crashed)" ;;
	135) echo "Bus error (4.2 BSD)" ;;
	136) echo "Floating point exception (e.g. divide by zero)" ;;
	137) echo "SIGKILL; likely an unrelated batch system kill." ;;
	139) echo "Segmentation violation (ANSI)" ;;
	143) echo "Termination (ANSI)(or incorporate in the msg text)" ;;
	147) echo "Error during attempted file stageout." ;;
	151) echo "Error during attempted file stageout." ;;
	152|-152) echo "CPU limit exceeded (4.2 BSD)" ;;
	153|-153) echo "File size limit exceeded (4.2 BSD)" ;;
	154|-154) echo "You cancelled the job" ;;
	155) echo "Profiling alarm clock (4.2 BSD)" ;;
	0)   echo "Still running (or unknown failure)" ;;
	*)   echo "unknown" ;;
    esac
}

for chunk in Chunk_* ; do
    fail=0
    description=""
    
    # check that root file is existing and not empty
    filename=${chunk}/seedRebuild.root
    if [ ! -e $filename ] ; then
	echo "Missing root file in " ${chunk}
	fail=1
    elif [ -z $filename ] ; then
	echo "Empty file: " $filename
        fail=1
    fi
    
    # Check job exit status. Cf. https://twiki.cern.ch/twiki/bin/view/CMSPublic/JobExitCodes , https://twiki.cern.ch/twiki/bin/view/CMSPublic/StandardExitCodes
    exitStatus=0
    if [ -s ${chunk}/exitStatus.txt ] ; then
	exitStatus=$(cat ${chunk}/exitStatus.txt)
	fail=1
    elif [ ! -e ${chunk}/exitStatus.txt ] ; then
	description="terminated, unknown failure"
	fail=1
    fi
    
    # Check for failures reported in the Condor log
    if [ "$exitStatus" -eq 0 ] ; then 
	logFile=${chunk}/log/*.log
	if [ -e $logFile ] ; then
	    if grep -q -e "Job removed.*due to wall time exceeded" $logFile ; then
		exitStatus=-152
		fail=1
	    elif grep -q -e "The job attribute PeriodicRemove expression.*evaluated to TRUE" $logFile ; then
		exitStatus=-153
		fail=1
	    elif grep -q -e "Job was aborted by the user" $logFile ; then
		exitStatus=-154
		fail=1
	    fi
	else
	    description="still running"
	    fail=$1
	fi
    fi
    
    # Archive succesful jobs, or report failure
    if [ $fail -eq 0 ] ; then
	# echo $chunk ": Done!"
	mkdir -p $gooddir
	mv $chunk $gooddir/
    else
	echo fail $fail
	[ -n "$description" ] && description=$(get_cause $exitStatus)
	echo $chunk ": failed, exit status = " $exitStatus $description
    fi
done

