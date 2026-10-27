Name:           qsstv
Version:        %{?qsstv_version}%{!?qsstv_version:9.6.0}
Release:        1%{?dist}
Summary:        Amateur radio SSTV and digital image application
License:        GPL-3.0-or-later
URL:            https://github.com/ploeffler/QSSTV
BuildArch:      x86_64
AutoReqProv:    yes

%description
QSSTV receives and transmits SSTV and digital image modes.
This is an experimental package.

%prep

%build

%install
install -Dm755 %{_sourcedir}/qsstv %{buildroot}%{_bindir}/qsstv
install -Dm644 %{_sourcedir}/qsstv.desktop %{buildroot}%{_datadir}/applications/qsstv.desktop
install -Dm644 %{_sourcedir}/qsstv.png %{buildroot}%{_datadir}/icons/hicolor/128x128/apps/qsstv.png

%files
%{_bindir}/qsstv
%{_datadir}/applications/qsstv.desktop
%{_datadir}/icons/hicolor/128x128/apps/qsstv.png

%changelog
* Tue Oct 27 2026 QSSTV contributors <noreply@github.com> - 9.6.0-1
- Integrate MMSSTV single-template and batch import

* Mon Oct 26 2026 QSSTV contributors <noreply@github.com> - 9.5.8-1
- Experimental Fedora RPM package
