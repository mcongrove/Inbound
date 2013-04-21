<?php
ini_set("display_errors", 1);

// Define constants
define("AU", 149597871); // Distance from Sun to Earth (1AU)
define("LED_COUNT", 7); // Number of LEDs between Sun and Earth on our hardware.

// Retrieve data from Royal Observatory of Belgium's CACTus tool, which utilizes data from the STEREO satellites
function getData()
{
	$ch = curl_init();

	curl_setopt($ch, CURLOPT_URL, "http://www.sidc.oma.be/cactus/out/cmecat.txt");
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);

	$output = curl_exec($ch);

	curl_close($ch);

	return $output;
}

// Parse the CACTus data, looking for CMEs
function parseData($data)
{
	// Ignore all the headers
	$split = preg_split("/.*#\s*CME\s*\|[^?]*\?\n/", $data);
	$split = preg_split("/\s*#\sFlow\s*\|[^?]*\?\n/", $split[1]);

	// Merge the CME and Flare data
	$split = $split[0] . "\n" . $split[1];

	// Split on the lines
	$lines = explode(PHP_EOL, $split);

	return $lines;
}

// Take the data string and make it into a usable object
function sanitizeData($data)
{
	$events = Array();

	// Loop through each event
	foreach ($data as $line) {
		$object = new stdClass();

		// Split on the pipes and remove empty values
		$columns = preg_split("/\s*\|\s*/", $line);
		$columns = array_filter($columns, "strlen");

		foreach ($columns as $i => $column) {
			switch ($i) {
				case 1:
					// Liftoff time converted to unix
					$object->liftoff = strtotime($column);
					break;
				case 4:
					// Angular width
					$object->width = $column;
					break;
				case 5:
					// Median velocity
					$object->velocity = $column;
					break;

			}
		}

		// Normally, we'd check for a partial or full-halo event here (+180 degrees?)
		// We should also be checking to see if the principal angle shows that the event will interact with Earth
		// For the sake of showing interesting data, we omit this check
		// Instead, we'll look for an angular width of only 90 degrees (partial halo) and not care if the event is coming our direction
		if (isset($object->width) && $object->width >= 15) {
			$events[] = $object;
		}
	}

	// Quick Sort helper
	function liftoffSort($a, $b) {
		return $a->liftoff < $b->liftoff;
	}

	uasort($events, 'liftoffSort');

	return $events;
}

// Organizes data based on logic needed for LED lights, etc.
function organizeData($data)
{
	$currentTime = time();
	$lights = array();

	foreach($data as $event) {
		$totalTime = doMath($event);
		$diff = round(($currentTime - $event->liftoff));
		$progress = $totalTime / $diff;
		$currentLight = LED_COUNT * $progress;
		$filter = (LED_COUNT + 1) * $progress;

		// Only worry about flares/CME's between the sun and earth
		if($filter <= 8) {
			$light = mapLight(round($currentLight));

			if($light == '!') {
				$firstIndexValue = null;
				if(isset($lights[0])) {
					$firstIndexValue = $lights[0];
				} else {
					$firstIndexValue = '';
				}
				$lights[0] = '!' . $firstIndexValue;
			} else {
				$lights[] = $light;
			}
		}
	}

	return implode(",", array_unique($lights));
}

// Map light to array used by arduino
function mapLight($light)
{
	$arduinoLights = array(
		0 => 7,
		1 => 6,
		2 => 5,
		3 => 4,
		4 => 9,
		5 => 10,
		6 => 11,
		7 => 12,
		8 => '!'
	);
	return $arduinoLights[$light];
}

// Does math to determine how long it takes for an LED to move across the hardware
function doMath($data)
{
	$seconds = round(AU / $data->velocity);
	$interval = round($seconds / LED_COUNT);

	return $interval;
}

// Go!
$newData = getData();
$newData = parseData($newData);
$events = sanitizeData($newData);
$formattedEvents = organizeData($events);

//echo '<pre>';
//print_r($events);
//echo '</pre>';
//exit();

// Output the data
echo "{" . $formattedEvents . "}";