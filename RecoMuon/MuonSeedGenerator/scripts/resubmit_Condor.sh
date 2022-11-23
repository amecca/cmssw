#/bin/sh

# (Re)submit all the chunks in a directory

# Make the grid proxy available in ~, if existing and valid
uid=$(id -u)
proxy_valid=$(voms-proxy-info --timeleft)
if [ $proxy_valid -gt 10 ] ; then
   echo "GRID proxy found, validity: $proxy_valid s"
   if [ -n "$X509_USER_PROXY" ] ; then
       [ "${X509_USER_PROXY}" = "~/x509up_u${uid}" ] || cp $X509_USER_PROXY ~/x509up_u${uid}
   elif [ -e /tmp/x509up_u${uid} ] ; then
     cp /tmp/x509up_u${uid} ~/
   fi
else # Last attempt: Try to see if a valid proxy already exists in ~
   if [ -e ~/x509up_u${uid} ] ; then
      export X509_USER_PROXY=~/x509up_u${uid}
      proxy_valid=`voms-proxy-info --timeleft`
      if [ $proxy_valid -gt 10 ] ; then
         echo "GRID proxy found in ~, validity: $proxy_valid s"
      fi
   fi

   if [ $proxy_valid -lt 10 ] ; then
      echo "Error: no valid GRID proxy found."
      exit 1
   fi
fi

if [ $# -ge 1 ] ; then
    cd ${1%condor.sub}
fi

queue='-queue directory in'

for x in Chunk* ; do
    if [ -e ${x}/log/*.log ] ; then
    	printf "${x}: job already submitted. If you want to resubmit, ensure all jobs are finished and run cleanup.csh.\nAborting.\n"
    	exit 1
    fi
    queue="$queue $x"
done

#set -x
condor_submit condor.sub $queue
