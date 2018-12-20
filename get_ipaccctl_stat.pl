#!/usr/bin/perl -w

use Time::localtime;
use POSIX ":sys_wait_h";

my @listen_interf = ('rl0','rl1');
my $iface_set = "no";
my @ng_modules;
my $ng_modules_def = "netgraph,ng_ether,ng_socket,ng_tee,ng_ipacct";
my $threshold = 5000;
my $ipacct_log = 'ng.log';
my $ipacct5min_log = '/usr/local/script/ng_stat/log/ng5min.log';
my @ipacct_arr;
my @ipacct_arr_in;

#########################
# Проверяем время.
#########################
$gm = localtime();
$year = ($gm->year()) + 1900;
$mounth = ($gm->mon()) + 1;
$mday = $gm->mday();
$date = "$mday-$mounth-$year";
$hour = $gm->hour();
$min = $gm->min();
$sec = $gm->sec();
$hour=sprintf("%02d",$hour);
$min=sprintf("%02d",$min);
$sec=sprintf("%02d",$sec);
$time = "$hour\:$min\:$sec";
#$table_date = "$year\_$mounth";

if (!defined $listen_interf[0]) {
    print "Установите пожалуйста в режим прослушивания хотя бы один интерфейс.\n";
}else{

	while (@listen_interf){
		$interface = shift @listen_interf;
		my $pid;
		$pid = fork;
		if (defined $pid) {
			if ($pid == 0){
				exec "/usr/local/sbin/ipacctctl $interface\_ip_acct:$interface checkpoint" or die "Ошибка передачи записи в checkpoint-базу!\n";
				exit;
			}
		}
		else {
			print "Фатальная ошибка ветвления!\n.................\n";
			die "Разделение на процессы не возможно.\n Принудительный выход из дочернего процесса: $!\n";
		}
		do {
			$kid = waitpid $pid,0;
			if ($kid == -1) {
			print "Дочерних процессов в системе нет или система не поддерживает их.\n Ошибка!" and die "Выход!\n";
			} elsif ($kid == 0) {
				print "Задан не блокирующий вызов и процесс еще не завершен!\n";
			}
		} until $kid=$pid;
		
		undef $pid;
		
		$pid = fork;
		if (defined $pid) {
			if ($pid == 0){
			exec "/usr/local/sbin/ipacctctl $interface\_ip_acct:$interface show >> $ipacct_log\.$interface" or die "Ошибка передачи записей из checkpoint-базы в файл!\n";
				exit;
			}
		}
		else {
			print "Фатальная ошибка ветвления!\n.................\n";
			die "Разделение на процессы не возможно.\n Принудительный выход из дочернего процесса: $!\n";
		}
		do {
			$kid = waitpid $pid,0;
			if ($kid == -1) {
			print "Дочерних процессов в системе нет или система не поддерживает их.\n Ошибка!" and die "Выход!\n";
			} elsif ($kid == 0) {
				print "Задан не блокирующий вызов и процесс еще не завершен!\n";
			}
		} until $kid=$pid;
		
		undef $pid;
		
		$pid = fork;
		if (defined $pid) {
			if ($pid == 0){
			exec "/usr/local/sbin/ipacctctl $interface\_ip_acct:$interface clear" or die "Ошибка при очистке checkpoint-базы! \nБаза не очищена. Возможно переполнение. Очистите базу в ручную\n";
				exit;
			}
		}
		else {
			print "Фатальная ошибка ветвления!\n.................\n";
			die "Разделение на процессы не возможно.\n Принудительный выход из дочернего процесса: $!\n";
		}
		do {
			$kid = waitpid $pid,0;
			if ($kid == -1) {
			print "Дочерних процессов в системе нет или система не поддерживает их.\n Ошибка!" and die "Выход!\n";
			} elsif ($kid == 0) {
				print "Задан не блокирующий вызов и процесс еще не завершен!\n";
			}
		} until $kid=$pid;
		
		undef $pid;
			
		$TMPLOG= "$ipacct_log\.$interface";
		open (TMPLOG, "$TMPLOG");
		$TMPLOG =~ s/\||`|&&|<|>//gi; #Очистка ряда символов | ` && < > из пути к файлу.
		while (<TMPLOG>){
			$tmp_log_line=$_;		
			chomp $tmp_log_line;
			$tmp_log_line = "$tmp_log_line $date $time $interface";
			push @ipacct_arr,$tmp_log_line;
		}
		close (TMPLOG);
		truncate ($TMPLOG,0);
		
		
		undef $pid;
	}

	open (IPCTLOG,">>$ipacct_log");
	while (@ipacct_arr){
		$line_arr = shift @ipacct_arr;
		$line_arr =~ s/\x20/\x09/g;
		$line_arr = "$line_arr\n";
		print IPCTLOG "$line_arr";	
	
	}
	close(IPCTLOG);	

	&ins_data_to_db;
	exec "/usr/local/script/ng_stat/traf5min" or die "Ошибка при выполнении заполнения таблицы trafmnf\n";
}

sub ins_data_to_db{
open F, "<$ipacct_log" || die "Can't open file $ipacct_log\n";
open F5MIN, ">$ipacct5min_log" || die "Can't open file $ipacct5min_log\n"; 
while ($str=<F>){
 last if(length($str)<10); 
 chomp $str;
 my @a;
 @a=split("\t",$str);
 my $ipsrc=$a[0];
 my $prsrc=$a[1];
 my $ipdst=$a[2];
 my $prdst=$a[3];
 my $proto=$a[4];
 my $pacts=$a[5];
 my $bytes=$a[6];
 my @dt=split("-",$a[7]);
 my $dtrec="$dt[2]-$dt[1]-$dt[0]";
 my $tmrec=$a[8];
 my $intfs=$a[9];
 if($ipsrc eq ""){
 }else{
  print F5MIN "$ipsrc;$prsrc;$ipdst;$prdst;$proto;$pacts;$bytes;$dtrec;$tmrec;$intfs\n";
 }
}
close F;
close F5MIN;
truncate ("$ipacct_log",0);
}
