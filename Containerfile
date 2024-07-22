FROM registry.access.redhat.com/fedora:latest

RUN  dnf module list --disablerepo=* --enablerepo=codeready-builder-for-rhel-9-x86_64-rpms \
     && dnf install --disableplugin=subscription-manager -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-9.noarch.rpm \
     && dnf update --disableplugin=subscription-manager -y \
     && dnf --disableplugin=subscription-manager -y --enablerepo=codeready-builder-for-rhel-9-x86_64-rpms install \
       gcc git \
       libcmocka-devel libedit-devel libevent-devel make meson \
       ninja-build numactl-devel pkgconf python3-pyelftools scdoc \
       libsmartcols-devel \
     && dnf --disableplugin=subscription-manager clean all

RUN git clone https://github.com/christophefontaine/grout && cd grout && make


FROM registry.access.redhat.com/ubi9:9.4
RUN dnf update --disableplugin=subscription-manager -y \
    && dnf --disableplugin=subscription-manager -y install \
       numactl-libs libedit libevent  \
    && dnf --disableplugin=subscription-manager clean all
COPY --from=0 /grout/build/grout /bin/grout
COPY --from=0 /grout/build/grcli /bin/grcli
COPY --from=0 /grout/build/subprojects/ecoli/src/libecoli.so /lib64/
CMD ["/bin/grout"]
