
$portName = $args[0]
$baudrate = $args[1]

Write-Host -ForegroundColor Yellow "Sending SIGINT (CTRL-C) to serial '$portName' @ '$baudrate' Baud..."
$port = new-Object System.IO.Ports.SerialPort $portName, $baudrate, None, 8, one

$port.Open()

# write CTRL-C (ascii 3) to serial port
$port.Write([byte[]](3), 0, 1)

$port.Close()
