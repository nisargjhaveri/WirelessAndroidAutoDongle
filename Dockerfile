FROM ubuntu

# RUN sed -i 's|deb http://us.archive.ubuntu.com/ubuntu/|deb mirror://mirrors.ubuntu.com/mirrors.txt|g' /etc/apt/sources.list
# RUN pkg --add-architecture i386
RUN apt-get -q update
RUN apt-get purge -q -y snapd lxcfs lxd ubuntu-core-launcher snap-confine
RUN apt-get -q -y install file wget cpio rsync build-essential libncurses5-dev \
		git bzr cvs mercurial subversion libc6 unzip bc \
		python3-setuptools
RUN apt-get -q -y autoremove && apt-get -q -y clean
# RUN update-locale LC_ALL=C