<?PHP
  $url_refresh=""; 
  $values = yaml_parse(file_get_contents("/var/lib/thermobzizou/status.yaml"));
  if($_GET['setpoint'] != '' && $_GET['setpoint'] != 0) {
    $setpoint=$_GET['setpoint'];
    if($setpoint != $values['SA']) {
      file_put_contents("/var/lib/thermobzizou/t.dat",$setpoint);
    }else{
      # Empty the ?setpoint=X part as we are now synchronous
      $url_refresh=";url=".$_SERVER["SCRIPT_URI"];
    }
  }else{
    $setpoint=$values['SA'];
  };
?>

<HEAD>
<TITLE>Bzizou's home heater control</TITLE>
<meta http-equiv="refresh" content="10<?PHP print $url_refresh;?>"> 
<style>

body { font-family: courrier; }
td { font-size: xx-large; }
A:link {background: #FFCC00; text-decoration: none}
A:visited {background: #FFCC00; text-decoration: none}
A:active {background: #FFCC00; text-decoration: none}
A:hover {background: #FFCC00; font-weight:bold; color: red;}

</style>
</HEAD>

<BODY BGCOLOR=000000 text=FFFFFF alink="#ffffff" link="#005500" vlink="#005500">


<CENTER>
<TABLE WIDTH=90% ALIGN=CENTER BGCOLOR=222222 BORDER=0 CELLSPACING=30>

<TR>
  <TD COLSPAN=2 ROWSPAN=2 VALIGN=MIDDLE ALIGN=CENTER>
    <FONT size=+5><B><?PHP print $values['TA'] ?></B></FONT>
   </B>&deg;C</FONT>
   <?PHP if ($values['AL'] != 0) { print "<BR><B><FONT COLOR=#FF0000>ALARM!</FONT></B>"; } ?>
  </TD>
  <TD COLSPAN=2 VALIGN=MIDDLE ALIGN=CENTER BGCOLOR=444444>
    <FONT size=+3><B>
    <?PHP 
      if ($setpoint == $values['SA']) {
        print $values['SA'];
      }else{
        print $setpoint;
        print "<FONT COLOR=#FF0000>*</FONT>";
      } 
    ?> 
    </B></FONT>
  </TD>
</TR>
<TR>
  <TD VALIGN=MIDDLE ALIGN=CENTER WIDTH=80>
   <A HREF=index.php?setpoint=<?PHP echo $setpoint-1; ?>>&nbsp;&nbsp;-&nbsp;&nbsp;</A>
  </TD>
  <TD VALIGN=MIDDLE ALIGN=CENTER WIDTH=80>
   <A HREF=index.php?setpoint=<?PHP echo $setpoint+1; ?>>&nbsp;+&nbsp;</A>
<TR>
  <TD VALIGN=MIDDLE ALIGN=CENTER>
    Exterieur<BR><?PHP print $values['TO'] ?>
  </TD>
  <TD VALIGN=MIDDLE ALIGN=CENTER>
    Garage<BR><?PHP print $values['TG'] ?>
  </TD>
  <TD COLSPAN=2 VALIGN=MIDDLE ALIGN=CENTER BGCOLOR=444444>
    Radiateurs<BR><?PHP print $values['TW'] ?>
  </TD>
</TR>
<TR>
  <TD VALIGN=MIDDLE ALIGN=CENTER>...</TD>
  <TD VALIGN=MIDDLE ALIGN=CENTER>...</TD>
  <TD COLSPAN=2 VALIGN=MIDDLE ALIGN=CENTER BGCOLOR=444444>
    Consigne: <?PHP print round($values['SW']) ?><BR>
    Servo: <?PHP print $values['SP'] ?>
  </TD>
</TR>
<TR>
  <TD COLSPAN=2 VALIGN=MIDDLE ALIGN=CENTER>
    <?PHP if (time() - $values['DATE'] > 60) { $color="#FF0000"; }
          else { $color="#FFFFFF";}
    ?>
    <FONT SIZE=-1 COLOR=<?PHP print $color?>>
      <?PHP print $values['DATE_HUMAN'] ?></FONT>
  </TD>
  <TD VALIGN=MIDDLE ALIGN=CENTER BGCOLOR=444444>
    <A HREF=http://www.bzizou.net/cacti/graph_view.php?action=tree&tree_id=2>
    <IMG BORDER=0 SRC=graph-icon3.png WIDTH=80></A>
  </TD> 
  <TD VALIGN=MIDDLE ALIGN=CENTER BGCOLOR=444444>
    <?PHP if ($values['HEATER'] == 0) { $icon="heat-icon-off.gif"; }
          else { $icon="heat-icon.gif"; }
    ?>
    <IMG ALIGN=CENTER SRC=<?PHP print $icon; ?>>
  </TD>
</TABLE>
</CENTER>   
</BODY>
