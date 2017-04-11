

echo "New remote server DNS setup and configuration"

echo ""

echo "You must run this as root from server.thecastledoctrine.net"

echo ""


echo -n "Enter IP address of remote server: "

read address


echo -n "Enter subdomain to use for remote server: "

read subdomain


echo ""

echo "Configuring DNS for $subdomain.onehouronelife.com to point to $address"

echo "Hit [ENTER] when ready: "
read

timestamp="$(date +"%s")"


cp /var/named/onehouronelife.com.db /var/named/onehouronelife.com.db_backup_$timestamp

echo "$subdomain	14400	IN	A	$address" >> /var/named/onehouronelife.com.db


echo "Triggering DNS reload"


rndc reload onehouronelife.com in internal

rndc reload onehouronelife.com in external


su jcr15<<EOSU

cd ~/checkout/OneLifeWorking
hg pull
hg update
cd scripts

scp linodeRootSetup.sh root@$address:

ssh root@address './linodeRootSetup.sh'

cd ~/.ssh

echo "" >> config
echo "Host           $subdomain" >> config
echo "HostName       $subdomain.onehouronelife.com" >> config
echo "IdentityFile   ~/.ssh/remoteServers_id_rsa" >> config
echo "User           jcr13" >> config

cd ~/www/reflector

echo "jcr13 $subdomain.onehouronelife.com 8005" >> remoteServerList.ini

EOSU