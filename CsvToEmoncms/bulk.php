<?php
/*
http://localhost/emoncms/input/bulk.json?data=[[1480115360,16,1137],[1480115361,16,1437,3164],[1480115362,16,1412,3077]]&time=0

2017-02-20T14:17:34,19.94,32.81,27.75,22.63,19.94,12.56

*/

$apikey = 'CHANGEME';
$host   = 'http://dhmonitor.jodaille.org';

// Filename as parameter
$file    = $argv[1];;
$row     = 1;
$nodeId  = 16;
$baseUrl = $host . "/input/bulk.json?time=0&apikey=$apikey&data=";
$newBulk = true;
if (($handle = fopen($file, "r")) !== FALSE) {
    while (($data = fgetcsv($handle, 10000, ",")) !== FALSE) {
        if($newBulk) $url = $baseUrl . '[';
        $newBulk = false;
        $num = count($data);
        //echo "<p> $num champs à la ligne $row: <br /></p>\n";
        $row++;

        for ($c=0; $c < $num; $c++) {
            if($c == 0) $url .= "[" . dateTimeToTimestamp($data[0]) . ",$nodeId,";
            else
            {
                if(!empty($data[$c]))
                {
                    $url .= $data[$c];
                    if($c < $num ) $url .=  ",";
                }

            }
        }
        $url = rtrim($url, ',');
        $url .= "],";
        $length = strlen($url);
        /*if($length > 200000)
        {
            $url .= ']';
            $newBulk = true;
            echo "GET $url\n";
        }*/
        echo "length: " . $length . "\n";
        //echo file_get_contents($url) . "\n";//exit;
    }
    $url = rtrim($url, ',');
    $url .= ']';
    echo "GET $url\n";
    fclose($handle);
}


function dateTimeToTimestamp($datetime)
{
    $d = new DateTime($datetime);
    return $d->getTimestamp();;
}
