#! /bin/sh
# --------------------------------------------------------------------- #
#
#  Script to run an example through VALGRIND and generate suppressions
#
#  Michael Hagemann <michael.hagemann@unibas.ch>
#
#  modified by Lorenzo Bettini <http://www.lorenzobettini.it>
#
# --------------------------------------------------------------------- #


# Hack to run valgrind with new glibcs.  Problem: new TLS (thread
# local storage)

#VG_ENV="LD_ASSUME_KERNEL=2.2.5"

# --num-callers=20 --leak-check=yes --leak-resolution=high --show-reachable=yes --suppressions=./suppressions.supp
# --num-callers=20 --leak-resolution=high  --suppressions=/home/bettini/work/src2tags/tests/suppressions.supp

VG_PRG=""
VG_ARGS="--tool=memcheck --num-callers=20 --leak-check=yes --leak-resolution=high --show-reachable=yes --suppressions=./suppressions.supp --gen-suppressions=yes"
#VG_ARGS="--tool=memcheck --num-callers=20 --leak-check=yes --leak-resolution=high --suppressions=./suppressions.supp"

VGRIND="${VG_ENV} ${VG_PRG} ${VG_ARGS}"

if test ! -x "${VG_PRG}"; then
    echo Valgrind not found!  Check path.
    exit 1
fi

# --------------------------------------------------------------------- #

DATE=`date +"%Y-%m-%d_%H%M"`
SUMMARY=valgrind_summary.log
TMP_LOG=valgrind_tmp.log
ERROR=0

# --------------------------------------------------------------------- #

vgrind () {
    if test ! -x "$1" -o "$1" == `basename $0`; then
	echo "Skipping $1."
	return
    fi

    echo -n "Running $* ..."
    tmp_err=

    if [[ $1 == *.sh ]]; then
        eval $1 ${VGRIND}
    else
        eval ${VGRIND} $*
    fi;

    #grep -e "LEAK SUMMARY" ${TMP_LOG} >/dev/null 2>&1
    grep -E "(reachable|lost): [1-9][0-9]*" ${TMP_LOG} >/dev/null 2>&1
    if test "$?" == "0"; then
	echo -n " LEAKS!"
	
	echo ""              >> ${SUMMARY}
	echo "** $*, LEAKS"  >> ${SUMMARY}
	cat ${TMP_LOG}       >> ${SUMMARY}
	tmp_err=1
    fi

    grep -e "[1-9][0-9]* error" ${TMP_LOG} >/dev/null 2>&1
    if test "$?" == "0"; then
	echo -n " ERRORS!"

	echo ""              >> ${SUMMARY}
	echo "** $*, ERRORS" >> ${SUMMARY}
	cat ${TMP_LOG}       >> ${SUMMARY}
	tmp_err=1
    fi

    if test "x${tmp_err}" == "x"; then
	echo " OK."
    else
	ERROR=1
	echo ""
    fi

    rm -f ${TMP_LOG}
}


# --------------------------------------------------------------------- #

echo "Run at ${DATE}" >${SUMMARY}

vgrind $*


# --------------------------------------------------------------------- #

# cat ${SUMMARY}

exit ${ERROR}
