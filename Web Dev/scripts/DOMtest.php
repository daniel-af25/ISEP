<?php
libxml_use_internal_errors(true);
$htmlDoc = new DOMDocument();
$htmlDoc->loadHTMLFile("uti3.html");
//echo $htmlDoc->saveHTML();
$id = 'orderID';
echo $htmlDoc->getElementbyID($id)->nodeValue = "changed again";
echo $htmlDoc->saveHTML();
// $x = $htmlDoc->documentElement;
// foreach ($x->childNodes AS $item) {
// $item->nodeValue="CHANGED";
// print $item->nodeName . " = " . $item->nodeValue . "<br>";
// }

?>