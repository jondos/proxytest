Summary: Mix Server of the AN.ON project
Name: Mix
Version: 00.03.79
Release: 1
URL: http//anon.inf.tu-dresden.de/
Source0: mix.src.tgz
License: BSD
Group: Networking
BuildRoot: %{_tmppath}/%{name}-root
BuildRequires: gcc gzip gcc-c++ binutils glibc-devel openssl-devel Xerces-c-devel Xerces-c libstdc++-devel libstdc++ make


%description
This is the Mix server, which is part of the AN.ON project

%prep
%setup -q -n proxytest

%build
%{configure}
make DESTDIR=${RPM_BUILD_ROOT}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=${RPM_BUILD_ROOT}
install -d ${RPM_BUILD_ROOT}/etc/mix
install -d ${RPM_BUILD_ROOT}/etc/init.d/rc2.d
install -d ${RPM_BUILD_ROOT}/etc/init.d/rc3.d
install -d ${RPM_BUILD_ROOT}/etc/init.d/rc5.d

install -o root -g root -m u+x -D documentation/init.d/mix ${RPM_BUILD_ROOT}/etc/init.d/mix
ln -s ../mix ${RPM_BUILD_ROOT}/etc/init.d/rc2.d/S70mix
ln -s ../mix ${RPM_BUILD_ROOT}/etc/init.d/rc3.d/S70mix
ln -s ../mix ${RPM_BUILD_ROOT}/etc/init.d/rc5.d/S70mix


%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/usr/sbin/mix
/etc/mix
/etc/init.d/mix
/etc/init.d/rc2.d/S70mix
/etc/init.d/rc3.d/S70mix
/etc/init.d/rc5.d/S70mix

%changelog
*Tue Jun 7 2005 JAP-Team <jap@inf.tu-dresden.de> 
-Initial build
