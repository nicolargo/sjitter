Summary: Sjitter provides, in a simple command line interface, a tool to measure jitter, one way delay and bandwidth.
Name: sjitter
Version: 0.15
Release: 1
License: GPL
Group: Applications/Internet
Source: http://www.alcasat.net/dev/sjitter/sjitter-0.14b.tgz
BuildRoot: /var/tmp/%{name}-buildroot

%description
Sjitter is a simple network performance tool to measure:
- jitter (based on the RFC 1889).
- one way delay (based on UDP datagram transit time).
- bandwidth (steel experimental).
Sjitter is a client/server program. Both client and server 
need to be date and time synchronize.
Sjitter is distributed under the GPL license.

%prep
%setup -c

%build
%configure

make RPM_OPT_FLAGS="$RPM_OPT_FLAGS"

%install
rm -rf $RPM_BUILD_ROOT
make install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README TODO
/usr/bin/sjitterc
/usr/bin/sjitters
/usr/share/man/man1/sjitter.1.gz

%changelog
* Thu Oct 5 2006 Nicolas Hennion <nicolashennion@gmail.com>:
Version 0.15:
- Package with automake
Version 0.14:
- Use timer (-t option)
- Rewrite progress bar procedure
- Man pages
Version 0.13:
- add buffer size parameters (-w option)
- Fix a bug in the minimum value
- Send a SJITTER-STOP datagram when a client is CTRL-C (thanks to Daitau Aaron)
Version 0.12:
- Use DNS name additionnaly to IP address (OK 06/07/2006)
- Multiple measure on the server
- Bug fix
Version 0.11:
- Correct bug in the archive
* Thu Jul 24 2006 Nicolas Hennion <nicolashennion@gmail.com>:
- First beta version
