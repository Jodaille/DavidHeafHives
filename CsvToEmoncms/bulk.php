<?php
/*
http://localhost/emoncms/input/bulk.json?data=[[1480115360,16,1137],[1480115361,16,1437,3164],[1480115362,16,1412,3077]]&time=0

2017-02-20T14:17:34,19.94,32.81,27.75,22.63,19.94,12.56

*/

$apikey = 'CHANGEME';
//$apikey = 'e5e2f1e2c5cfc557b2889e9583894cf2';// APIKEY sample

$host   = 'http://dhmonitor.jodaille.org';
//$host   = 'http://localhost/emoncms';

// nodeId: 16 is Warré / 25 is Lanzutin
$nodeId  = 25;

// Filename as parameter
$file    = $argv[1];;

// time=0 is need for emoncms to use our date time not the server one
$baseUrl = $host . "/input/bulk.json?time=0&apikey=$apikey&data=";

// We start a new array when true
$newBulk = true;

$row     = 1;

// Opening file
if (($handle = fopen($file, "r")) !== FALSE) {
    // Get each lines
    while (($data = fgetcsv($handle, 10000, ",")) !== FALSE) {
        // Start of first array of data
        if($newBulk) $url = $baseUrl . '[';
        $newBulk = false;
        // How many columns found ?
        $num = count($data);
        $row++;

        // Get each columns values
        for ($c=0; $c < $num; $c++) {
            // start array of datas, convert to timestamp, adding node ID
            if($c == 0) {
                $url .= "[" . dateTimeToTimestamp($data[0]) . ",$nodeId,";
            }
            else {
                if(!empty($data[$c])) {
                    $url .= $data[$c];
                    if($c < $num ) $url .=  ",";
                }
            }
        }

        $url = rtrim($url, ','); // remove extra comma

        $url .= "],";// closing array

        $length = strlen($url);
        // 4000 seems to be max url length size (file_get_contents)
        if ($length > 4000) {
            $url = rtrim($url, ',');
            $url .= ']';
            $newBulk = true;
            echo "GET $url\n";
            echo file_get_contents($url) . "\n";// Query server
        }

    }
    $url = rtrim($url, ',');
    $url .= ']';

    fclose($handle);// close file
    echo file_get_contents($url) . "\n";
}


function dateTimeToTimestamp($datetime)
{
    $d = new DateTime($datetime);
    return $d->getTimestamp();;
}
