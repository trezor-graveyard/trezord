if which systemctl > /dev/null ; then
  systemctl enable trezord.service
  systemctl start trezord.service
else
  chkconfig --add trezord
  service trezord start
fi
