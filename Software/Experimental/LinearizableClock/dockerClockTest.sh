#!/bin/bash

stp=0
pull=0
hosts="oc1 oc2 tc gm bc"
image="docker.pkg.github.com/better-clocks/better_clocks/linearizable_clock:latest"
image_file="/tmp/image.tar.gz"

help="\n\
This script manages docker containers on selected hosts.\n\
See https://github.com/Better-Clocks/better_clocks#linearizable-clock for more.\n\
\n\
Usage:\n
\t$0\t\t\tRun tests using existing image.\n\
\t$0 -h\t\t\tDisplay this help message.\n\
\t$0 -p\t\t\tPull and deploy latest image from github. Default false\n
\t$0 -s\t\t\tStop tests and exit. Default: false\n
\t$0 -l\t\t\tSpecify list of hosts to run tests against. Default: $hosts\n
\t$0 -i\t\t\tSpecify docker image:version. Default: $image\n
\t$0 -d\t\t\tDebug mode\n
"
while getopts ":i:l:spdh" opt; do
  case ${opt} in
    h )
      echo -e ${help}
      exit 0
      ;;
    s )
      stp=1
      ;;
    p )
      pull=1
      ;;
    l )
      hosts=$OPTARG
      ;;
    i )
      image=$OPTARG
      ;;
    d )
      echo "Enabling debug output as requested"
      set -x
      ;;
    \? )
      echo "Invalid Option: -$OPTARG" 1>&2
      echo -e ${help}
      exit 1
      ;;
    : )
      echo "Invalid option: $OPTARG requires an argument" 1>&2
      echo -e ${help}
      exit 1
      ;;
  esac
done
shift $((OPTIND -1))

# Kill running test. We don't care if it fails
echo -n "Killing running tests..."
pssh -v -H "$hosts" "docker kill \$(docker ps -q  --filter ancestor=${image})" >/dev/null 2>&1 \
    && echo "Done" || echo "Nothing was running"
if [ $stp -eq 1 ]; then
    exit 0
fi

if [ $pull -eq 1 ]; then
    echo "Logging in docker repo. Please enter your github username and read-only token"
    echo "More details https://github.com/Better-Clocks/better_clocks#docker-registry"
    docker login docker.pkg.github.com
    docker pull ${image}
    echo "Logging out"
    docker logout docker.pkg.github.com
else
    echo "Not updating the image from github"
fi

echo "Checking if image exists on $hosts"
pssh -v -H "$hosts" "docker image inspect ${image}"
if [ $? -ne 0 ] || [ $pull -eq 1 ]; then
    if [ $pull -eq 0 ]; then
        echo "No requested image version stored on $hosts!"
    fi
    echo "Deploying"

    img=$(docker images ${image} -q)
    if [ -z $img ]; then
        echo "No requested image version stored locally!"
        exit 1
    fi

    # Deploy image
    image_size=$(docker image inspect ${image} --format='{{.Size}}')
    ## Save
    echo "Saving image to the file"
    docker save ${image} | pv -s ${image_size} | gzip -1 > ${image_file}
    ## Copy
    echo "Copy image to remote hosts. May take a minute..."
    pscp.pssh -v -H "$hosts" "${image_file}" "${image_file}"
    ## Import
    echo "Loading image to remote dockers"
    pssh -H "$hosts" "gunzip -c ${image_file} | docker load"

    # Cleanup old "none" images
    pssh -H "$hosts" "docker rmi \$(docker images -f 'dangling=true' -q) -f || true" 2>/dev/null 1>&2
fi

# Start image
starttime=$(date +%s)
echo "Starting test. See /var/log/dockerClockTest/${starttime} for details"

pssh -o /var/log/dockerClockTest/${starttime}/out -e /var/log/dockerClockTest/${starttime}/err -t 0 -H "$hosts" "hn=\$(hostname -s) ; echo \$hn ; ips=\$(echo $hosts | sed -e \"s/\$hn//g\") ; echo \$ips ; docker run --network host -v /dev:/dev -e \"IP=\$ips\" ${image}"
