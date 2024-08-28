#
# spec file for configuration of package apache
#
# Copyright  (c)  2010  Ericsson LM
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# please send bugfixes or comments to paolo.palmieri@ericsson.com
#

Name:      %{_name}
Summary:   Installation package for EvHandlClient.
Version:   %{_prNr}
Release:   %{_rel}
License:   Ericsson Proprietary
Vendor:    Ericsson LM
Packager:  %packer
Group:     Library
BuildRoot: %_tmppath
Requires: APOS_OSCONFBIN

%define EVHANDLCLIENTdir %{APdir}/ext/evhandlclient
%define EVHANDLCLIENTBINdir %{EVHANDLCLIENTdir}/bin

%define evhandlclient_bin_path %{_cxcdir}/bin

#BuildRoot: %{_tmppath}/%{name}_%{version}_%{release}-build

%description
Installation package for EvHandlClient.

%pre
if [ $1 == 1 ] 
then
echo "This is the %{_name} package %{_rel} pre-install script during installation phase"

fi
if [ $1 == 2 ]
then
echo "This is the %{_name} package %{_rel} pre-install script during upgrade phase"
rm -f /usr/bin/evhandlclient
fi

%install
echo "This is the %{_name} package %{_rel} install script"

mkdir -p $RPM_BUILD_ROOT/opt/ap/ext/evhandlclient/bin

if [ ! -d $RPM_BUILD_ROOT%rootdir ]
then
	mkdir $RPM_BUILD_ROOT%rootdir
fi
if [ ! -d $RPM_BUILD_ROOT%APdir ]
then
	mkdir $RPM_BUILD_ROOT%APdir
fi
if [ ! -d $RPM_BUILD_ROOT%EVHANDLCLIENTdir ]
then
	mkdir $RPM_BUILD_ROOT%EVHANDLCLIENTdir
fi
if [ ! -d $RPM_BUILD_ROOT%EVHANDLCLIENTBINdir ]
then
	mkdir $RPM_BUILD_ROOT%EVHANDLCLIENTBINdir
fi


cp %evhandlclient_bin_path/evhandlclient	$RPM_BUILD_ROOT/opt/ap/ext/evhandlclient/bin
cp %evhandlclient_bin_path/gmlog.sh	$RPM_BUILD_ROOT/opt/ap/ext/evhandlclient/bin
cp %evhandlclient_bin_path/rpmo.sh	$RPM_BUILD_ROOT/opt/ap/ext/evhandlclient/bin

%post
echo "This is the %{_name} package %{_rel} post-install script"
ln -sf $RPM_BUILD_ROOT%EVHANDLCLIENTBINdir/gmlog.sh $RPM_BUILD_ROOT/usr/bin/gmlog
ln -sf $RPM_BUILD_ROOT%EVHANDLCLIENTBINdir/rpmo.sh $RPM_BUILD_ROOT/usr/bin/rpmo

%preun
echo "This is the %{_name} package %{_rel} pre-uninstall script"


%postun
if [ $1 -eq 0 ] 
then
echo "This is the %{_name} package %{_rel} postun script during unistall phase --> start"
echo "remove links"
rm -f /usr/bin/gmlog
rm -f /usr/bin/rpmo

rm -f $RPM_BUILD_ROOT%EVHANDLCLIENTBINdir/evhandlclient
rm -f $RPM_BUILD_ROOT%EVHANDLCLIENTBINdir/gmlog.sh
rm -f $RPM_BUILD_ROOT%EVHANDLCLIENTBINdir/rpmo.sh
fi


if [ $1 -eq 1 ] 
then
echo "This is the %{_name} package %{_rel} postun script during upgrade phase"
fi


%files
%defattr(-,root,root)
%attr(555,root,root) %EVHANDLCLIENTBINdir/evhandlclient
%attr(555,root,root) %EVHANDLCLIENTBINdir/gmlog.sh
%attr(555,root,root) %EVHANDLCLIENTBINdir/rpmo.sh

%changelog
* Wed Jul 07 2010 - fabio.ronca (at) ericsson.com
- Initial implementation
