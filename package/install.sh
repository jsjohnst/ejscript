#!/bin/bash
#
#	install: Installation script
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#
# 	Usage: install [configFile]
#
################################################################################
#
#	The configFile is of the format:
#		FMT=[rpm|deb|tar]				# Package format to use
#		srcDir=sourcePath          		# Where to install the src
#		devDir=documentationPath		# Where to install the doc
#		installbin=[YN]					# Install binary package
#		installsrc=[YN]					# Install source code package
#		installdev=[YN]					# Install dev headers package
#

HOME=`pwd`
FMT=

BLD_PRODUCT="!!BLD_PRODUCT!!"
BLD_NAME="!!BLD_NAME!!"
BLD_VERSION="!!BLD_VERSION!!"
BLD_NUMBER="!!BLD_NUMBER!!"
BLD_HOST_OS="!!BLD_HOST_OS!!"
BLD_HOST_CPU="!!BLD_HOST_CPU!!"
BLD_HOST_DIST="!!BLD_HOST_DIST!!"

BLD_PREFIX="!!BLD_PREFIX!!"				# Fixed and can't be relocated
BLD_DOC_PREFIX="!!BLD_DOC_PREFIX!!"
BLD_INC_PREFIX="!!BLD_INC_PREFIX!!"
BLD_LIB_PREFIX="!!BLD_LIB_PREFIX!!"
BLD_SAM_PREFIX="!!BLD_SAM_PREFIX!!"
BLD_SBIN_PREFIX="!!BLD_SBIN_PREFIX!!"
BLD_SRC_PREFIX="!!BLD_SRC_PREFIX!!"

installbin=Y
installdev=Y
installsrc=N

PATH=$PATH:/sbin:/usr/sbin

###############################################################################
#
#	Initialization
#

setup() {

	umask 022

	if [ $BLD_HOST_OS != WIN -a `id -u` != "0" ] ; then
		echo "You must be root to install this product."
		exit 255
	fi

	#
	#	Headless install
	#
	if [ $# -ge 1 ] ; then
		if [ ! -f $1 ] ; then
			echo "Could not find installation config file \"$1\"." 1>&2
			exit 255
		else
			. $1 
			installFiles $FMT
		fi
		exit 0
	fi

	sleuthPackageFormat

	echo -e "\n$BLD_NAME !!BLD_VERSION!!-!!BLD_NUMBER!! Installation\n"

}


#
# 	Try to guess if we should default to using RPM
#
sleuthPackageFormat() {
	local name

	name=`createPackageName ${BLD_PRODUCT}-bin`
	FMT=
	for f in deb rpm tar.gz ; do
		if [ -f ${name}.${f} ] ; then
			FMT=${f%.gz}
			break
		fi
	done

	if [ "$FMT" = "" ] ; then
		echo -e "\nYou may be be missing a necessary package file. "
		echo "Check that you have the correct $BLD_NAME package".
		exit 255
	fi
}


askUser() {
	local finished

	echo "Enter requested configuration information or press <ENTER> to accept"
	echo -e "the defaults. "
	
	#
	#	Confirm the configuration
	#
	finished=N
	while [ "$finished" = "N" ]
	do
		echo
		installbin=`yesno "Install binary package" "$installbin"`
		installdev=`yesno "Install development headers and samples package" "$installdev"`
#		installsrc=`yesno "Install source code package" "$installsrc"`
	
		echo -e "\nInstalling with this configuration:" 
		echo -e "    Install binary package: $installbin"
        echo -e "    Install development doc, headers and samples package: $installdev"
#		echo -e "    Install source package: $installsrc"
	
		echo
		finished=`yesno "Accept this configuration" "Y"`
	done
	
	if [ "$installbin" = "N" -a "$installdev" = "N" -a "$installsrc" = "N" ] ; then
		echo -e "\nNothing to install, exiting. "
		exit 0
	fi
	
	#
	#	Save the install settings. Remove.sh will need this
	#
	saveSetup
}


createPackageName() {

	echo ${1}-${BLD_VERSION}-${BLD_NUMBER}-${BLD_HOST_DIST}-${BLD_HOST_OS}-${BLD_HOST_CPU}
}


# 
#	Get a yes/no answer from the user. Usage: ans=`yesno "prompt" "default"`
#	Echos 1 for Y or 0 for N
#
yesno() {
	local ans

	if [ "$!!BLD_PRODUCT!!_HEADLESS" = 1 ] ; then
		echo "Y"
		return
	fi
	echo -n "$1 [$2] : " 1>&2
	while [ 1 ] 
	do
		read ans
		if [ "$ans" = "" ] ; then
			echo $2 ; break
		elif [ "$ans" = "Y" -o "$ans" = "y" ] ; then
			echo "Y" ; break
		elif [ "$ans" = "N" -o "$ans" = "n" ] ; then
			echo "N" ; break
		fi
		echo -e "\nMust enter a 'y' or 'n'\n " 1>&1
	done
}


# 
#	Get input from the user. Usage: ans=`ask "prompt" "default"`
#	Returns the answer or default if <ENTER> is pressed
#
ask() {
	local ans

	default=$2

	if [ "$!!BLD_PRODUCT!!_HEADLESS" = 1 ] ; then
		echo "$default"
		return
	fi

	echo -n "$1 [$default] : " 1>&2
	read ans
	if [ "$ans" = "" ] ; then
		echo $default
	fi
	echo $ans
}


#
#	Save the setup
#
saveSetup() {
	local firstChar

	echo -e "FMT=$FMT\nbinDir=$BLD_PREFIX\ninstallbin=$installbin\ninstalldev=$installdev\ninstallsrc=$installsrc" >/etc/${BLD_PRODUCT}Install.conf
}


installFiles() {
	local dir pkg doins NAME upper

	echo -e "\nExtracting files ..."

	for pkg in bin dev ; do
		
		doins=`eval echo \\$install${pkg}`
		if [ "$doins" = Y ] ; then
			suffix="-${pkg}"
			#
			#	RPM doesn't give enough control on error codes. So best to keep going.	
			#
			NAME=`createPackageName ${BLD_PRODUCT}${suffix}`.$FMT
			if [ "$FMT" = "rpm" ] ; then
				echo -e "\nrpm -Uhv $NAME"
				rpm -Uhv $HOME/$NAME
			elif [ "$FMT" = "deb" ] ; then
				echo -e "\ndpkg -i $NAME"
				dpkg -i $HOME/$NAME >/dev/null
			else
				# cd /
				# echo -e "\ntar xfz ${NAME}.gz"
				# tar xfz $HOME/${NAME}.gz

				#
				#	Need to strip the top directory off. Not all tar versions support tar --strip=N yet
				#
				dir=/tmp/${BLD_PRODUCT}-$$
				mkdir -p $dir
				cd $dir
				echo -e "\ntar xfz ${NAME}.gz"
				tar xfz $HOME/${NAME}.gz
                upper=`echo ${pkg} | tr '[a-z]' '[A-Z]'`
				cd "${upper}" >/dev/null
				#
				#	Need to avoid any directories for redhat9 and so we don't change perms unnecessarily.
				#
				( find . -type f ; find . -type l) | xargs tar cf -  | tar xpf - -C /
				cd $HOME
				rm -fr $dir
			fi
		fi
	done

	if [ -f /etc/redhat-release -a -x /usr/bin/chcon ] ; then 
		sestatus | grep enabled >/dev/null 2>&1
		if [ $? = 0 ] ; then
			for f in $BLD_LIB_PREFIX/*.so ; do
				chcon /usr/bin/chcon -t texrel_shlib_t $f
			done
		fi
	fi

	if which ldconfig >/dev/null 2>&1 ; then
		ldconfig -n $BLD_LIB_PREFIX
	fi

	if [ $BLD_HOST_OS = WIN ] ; then
		echo -e "\nSetting file permissions ..."
		find "$BLD_PREFIX" -type d | xargs chmod 755 
		find "$BLD_PREFIX" -type f | xargs chmod g+r,o+r 
		chmod 777 "$BLD_PREFIX/logs"
	fi
	echo
}


#
#   Cleanup for 0.9.6 and before
#
legacyPrep() {
    rm -f /usr/bin/ejs.mod
    rm -f /usr/bin/ejs.db.mod
    rm -f /usr/bin/ejs.web.mod
    rm -f /usr/bin/ejsweb.mod

    rm -f /usr/bin/egen
    rm -f /usr/bin/ec
    rm -f /usr/bin/ecgi
    rm -f /usr/bin/ejs
    rm -f /usr/bin/ejsc
    rm -f /usr/bin/ejscgi
    rm -f /usr/bin/ejsmod
    rm -f /usr/bin/ejssql
    rm -f /usr/bin/ejsvm
    rm -f /usr/bin/ejsweb

    rm -f /usr/lib/ejs/ejs.mod
    rm -f /usr/lib/ejs/ejs.db.mod
    rm -f /usr/lib/ejs/ejs.web.mod
    rm -f /usr/lib/ejs/ejsweb.mod
}

###############################################################################
#
#	Main program for install script
#

setup $*

askUser

legacyPrep
installFiles $FMT

echo -e "\n$BLD_NAME installation successful.\n"
