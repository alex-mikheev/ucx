#!/bin/bash -eExl

rc=0

if [ -z "$BUILD_NUMBER" ]; then
    echo Running interactive
    WORKSPACE=$PWD
    BUILD_NUMBER=1
    JOB_URL=file://$WORKSPACE
else
    echo Running under jenkins
    JOB_URL=$JOB_URL/ws
fi

rpm_topdir=$WORKSPACE/rpm-dist

make_opt="-j$(($(nproc) - 1))"

echo Starting on host: $(hostname)

echo "Autogen"
./autogen.sh

echo "Making a directory for test build"
mkdir -p build-test
cd build-test

echo "Build release"
../contrib/configure-release && make $make_opt && make $make_opt distcheck

echo "Build gtest "
make clean && ../contrib/configure-devel && make $make_opt

echo "Starting gtest"

make -C test/gtest test

module load tools/valgrind-3.9.0
make -C test/gtest VALGRIND_EXTRA_ARGS="--xml=yes --xml-file=valgrind.xml" test_valgrind
module unload tools/valgrind-3.9.0


echo "Build with coverity"
module load tools/cov
cov_build_id="cov_build_${BUILD_NUMBER}"
cov_build="$WORKSPACE/$cov_build_id"
rm -rf $cov_build
make clean
cov-build --dir $cov_build make $make_opt all
nerrors=$(cov-analyze --dir $cov_build |grep "Defect occurrences found" | awk '{print $5}')
cov-format-errors --dir $cov_build
rc=$(($rc+$nerrors))

cov_url="$JOB_URL/$cov_build_id/c/output/errors/index.html"
rm -f jenkins_sidelinks.txt
echo 1..1 > coverity.tap
if [ $nerrors -gt 0 ]; then
    echo "not ok 1 Coverity Detected $nerrors failures # $cov_url" >> coverity.tap
else
    echo ok 1 Coverity found no issues >> coverity.tap
fi
echo Coverity report: $cov_url
printf "%s\t%s\n" Coverity $cov_url >> jenkins_sidelinks.txt
module unload tools/cov

#(make distcheck && rm -rf $rpm_topdir && mkdir -p $rpm_topdir && cd $rpm_topdir && mkdir -p BUILD RPMS SOURCES SPECS SRPMS)
#
#if [ -x /usr/bin/dpkg-buildpackage ]; then
#    echo "Build on debian"
#    dpkg-buildpackage -us -uc
#else
#    echo "Build rpms"
#    rpmbuild -bs --define '_sourcedir .' --define "_topdir $rpm_topdir" --nodeps mxm.spec
#    fn=$(find $rpm_topdir -name "*.src.rpm" -print0)
#    rpmbuild --define "_topdir $rpm_topdir" --rebuild $fn
#fi

exit $rc