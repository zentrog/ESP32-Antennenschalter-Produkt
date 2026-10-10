<?php
declare(strict_types=1);

/*
 * Antennensteuerung: privacy-conscious, two-step email report endpoint.
 * Requires PHP 7.4+ with OpenSSL. Keep mail-config.php out of the public
 * document root if the host allows it; otherwise protect it at the webserver.
 */
const MAX_BODY = 140000;
const REPORT_TO = 'andreas.bodyn@gmx.de';

header('Cache-Control: no-store, max-age=0');
header('X-Content-Type-Options: nosniff');
header('Referrer-Policy: no-referrer');
header('X-Frame-Options: DENY');
header("Content-Security-Policy: default-src 'none'; style-src 'unsafe-inline'; form-action 'self'; base-uri 'none'; frame-ancestors 'none'");
$isHealth = ($_SERVER['REQUEST_METHOD'] ?? '') === 'GET' && isset($_GET['health']);
if ($isHealth) header('Access-Control-Allow-Origin: *');

$configPath = getenv('ANTCTRL_REPORT_CONFIG') ?: __DIR__ . '/mail-config.php';
define('ANTCTRL_REPORT_CONFIG_INCLUDED', true);
if (!is_file($configPath) || !is_readable($configPath)) {
    fail(503, 'Der Mailversand ist auf diesem Server noch nicht eingerichtet.');
}
$cfg = require $configPath;
if (!is_array($cfg) || empty($cfg['host']) || empty($cfg['username']) || empty($cfg['password']) || empty($cfg['secret'])) {
    fail(503, 'Die Mailkonfiguration ist unvollständig.');
}
if ($isHealth) {
    if (!is_https()) health_result(false);
    try { rate_limit('health', 30); $s = smtp_open_authenticated($cfg); smtp_cmd($s, 'QUIT', [221]); fclose($s); health_result(true); }
    catch (Throwable $e) { error_log('Antennensteuerung report health check failed: ' . get_class($e)); health_result(false); }
}

if (($_SERVER['REQUEST_METHOD'] ?? '') !== 'POST') {
    fail(405, 'Bitte den Meldebutton in der Antennensteuerung verwenden.');
}
$https = (!empty($_SERVER['HTTPS']) && strtolower((string)$_SERVER['HTTPS']) !== 'off')
    || (string)($_SERVER['HTTP_X_FORWARDED_PROTO'] ?? '') === 'https';
if (!$https) fail(400, 'Der Meldeweg ist nur über eine verschlüsselte HTTPS-Verbindung verfügbar.');
$length = (int)($_SERVER['CONTENT_LENGTH'] ?? 0);
if ($length < 1 || $length > MAX_BODY) fail(413, 'Der Bericht ist leer oder zu groß.');
$raw = file_get_contents('php://input', false, null, 0, MAX_BODY + 1);
if ($raw === false || strlen($raw) > MAX_BODY) fail(413, 'Der Bericht ist zu groß.');
$form = [];
parse_str($raw, $form);
if (!empty($form['website'])) fail(400, 'Anfrage abgelehnt.'); // honeypot

if (isset($form['token'], $form['payload'])) {
    $payloadJson = (string)$form['payload'];
    $token = (string)$form['token'];
    $expected = hash_hmac('sha256', $payloadJson, (string)$cfg['secret']);
    if (!hash_equals($expected, $token)) fail(400, 'Die Bestätigung ist abgelaufen oder ungültig. Bitte den Bericht neu absenden.');
    $report = json_decode($payloadJson, true);
    if (!is_array($report)) fail(400, 'Der Bericht ist ungültig.');
    if (!isset($report['_issued']) || !is_int($report['_issued']) || $report['_issued'] > time() || $report['_issued'] < time() - 900) fail(400, 'Die Vorschau ist abgelaufen. Bitte den Bericht erneut prüfen.');
    rate_limit('report', 5);
    try {
        smtp_send($cfg, make_message($report));
    } catch (Throwable $e) {
        error_log('Antennensteuerung report delivery failed: ' . get_class($e));
        fail(502, 'Der Mailversand ist fehlgeschlagen. Bitte später erneut versuchen oder den Bericht lokal speichern.');
    }
    page('Bericht versendet', '<p>Vielen Dank. Der Fehlerbericht wurde an den Projektkontakt übermittelt.</p><p>Es wurden keine ESP-Konfiguration, WLAN-Zugangsdaten, IP-/MAC-Adresse oder Sicherungsdatei angehängt.</p>');
}

$report = [
    'category' => field($form, 'category', 40, true),
    'subject' => field($form, 'subject', 120, true),
    'description' => field($form, 'description', 5000, true),
    'steps' => field($form, 'steps', 3000, false),
    'expected' => field($form, 'expected', 2000, false),
    'since' => field($form, 'since', 2000, false),
    'version' => field($form, 'version', 40, false),
    'name' => field($form, 'name', 100, false),
    'contact' => field($form, 'contact', 200, false),
    '_issued' => time(),
];
if (!in_array($report['category'], ['Fehler', 'Wunsch', 'Sonstiges'], true)) fail(400, 'Bitte eine gültige Kategorie wählen.');
$payload = json_encode($report, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
$token = hash_hmac('sha256', $payload, (string)$cfg['secret']);
$hiddenPayload = htmlspecialchars($payload, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
$hiddenToken = htmlspecialchars($token, ENT_QUOTES, 'UTF-8');
$summary = '';
foreach ($report as $key => $value) {
    if ($key === '_issued') continue;
    if ($value !== '') $summary .= '<dt>' . esc(label($key)) . '</dt><dd>' . nl2br(esc($value), false) . '</dd>';
}
$html = '<p>Prüfe den Inhalt. Erst mit „Jetzt per E-Mail senden“ wird er übermittelt.</p><dl>' . $summary . '</dl>'
    . '<form method="post"><input type="hidden" name="payload" value="' . $hiddenPayload . '"><input type="hidden" name="token" value="' . $hiddenToken . '"><input type="text" name="website" value="" class="trap" tabindex="-1" autocomplete="off"><button>Jetzt per E-Mail senden</button></form>'
    . '<p>Es werden keine ESP-Konfiguration, WLAN-Zugangsdaten, IP-/MAC-Adresse oder Sicherungsdateien angehängt.</p>';
page('Bericht prüfen', $html);

function field(array $form, string $key, int $max, bool $required): string {
    $raw = $form[$key] ?? '';
    if (!is_string($raw) && !is_numeric($raw)) fail(400, 'Ein Eingabefeld ist ungültig.');
    $v = trim((string)$raw);
    if (strlen($v) > $max * 4) fail(400, 'Ein Eingabefeld ist zu lang.');
    $v = preg_replace('/[\x00-\x08\x0B\x0C\x0E-\x1F\x7F]/u', '', $v) ?? '';
    if (function_exists('mb_substr')) $v = mb_substr($v, 0, $max, 'UTF-8'); else $v = substr($v, 0, $max);
    if ($required && $v === '') fail(400, 'Bitte alle Pflichtfelder ausfüllen.');
    return $v;
}
function make_message(array $r): string {
    $labels = ['category'=>'Art','subject'=>'Betreff','description'=>'Beschreibung','steps'=>'Schritte','expected'=>'Erwartetes Verhalten','since'=>'Seit wann / nach welcher Änderung','version'=>'Firmware-Version','name'=>'Name','contact'=>'Rückkontakt'];
    $out = "Neuer Bericht zur Antennensteuerung\n\n";
    foreach ($labels as $k=>$label) if (!empty($r[$k])) $out .= $label . ":\n" . str_replace(["\r\n", "\r"], "\n", (string)$r[$k]) . "\n\n";
    return $out . "\nVersand über den bestätigten HTTPS-Meldeendpunkt. Keine Konfiguration angehängt.\n";
}
function smtp_open_authenticated(array $c) {
    $host = (string)$c['host']; $port = (int)($c['port'] ?? 465);
    if ($port !== 465) throw new RuntimeException('Unsupported SMTP port');
    $errno = 0; $errstr = '';
    $s = stream_socket_client('ssl://' . $host . ':' . $port, $errno, $errstr, 12, STREAM_CLIENT_CONNECT);
    if (!$s) throw new RuntimeException('SMTP connection failed');
    stream_set_timeout($s, 12);
    try {
        smtp_expect($s, [220]); smtp_cmd($s, 'EHLO antennensteuerung.local', [250]);
        smtp_cmd($s, 'AUTH LOGIN', [334]);
        smtp_cmd($s, base64_encode((string)$c['username']), [334]);
        smtp_cmd($s, base64_encode((string)$c['password']), [235]);
        return $s;
    } catch (Throwable $e) { fclose($s); throw $e; }
}
function smtp_send(array $c, string $body): void {
    $s = smtp_open_authenticated($c);
    try {
        $from = (string)($c['from'] ?? $c['username']);
        smtp_cmd($s, 'MAIL FROM:<' . smtp_addr($from) . '>', [250]);
        smtp_cmd($s, 'RCPT TO:<' . smtp_addr(REPORT_TO) . '>', [250, 251]); smtp_cmd($s, 'DATA', [354]);
        $subject = '=?UTF-8?B?' . base64_encode('[Antennensteuerung] Neuer Bericht') . '?=';
        $msg = 'From: Antennensteuerung <' . smtp_addr($from) . ">\r\nTo: " . REPORT_TO . "\r\nSubject: $subject\r\nMIME-Version: 1.0\r\nContent-Type: text/plain; charset=UTF-8\r\nContent-Transfer-Encoding: 8bit\r\n\r\n" . str_replace("\n", "\r\n", str_replace(["\r\n", "\r"], "\n", $body));
        $msg = preg_replace('/(?m)^\./', '..', $msg) . "\r\n.";
        fwrite($s, $msg . "\r\n"); smtp_expect($s, [250]); smtp_cmd($s, 'QUIT', [221]);
    } finally { fclose($s); }
}
function smtp_addr(string $v): string { if (!filter_var($v, FILTER_VALIDATE_EMAIL) || preg_match('/[\r\n<>]/', $v)) throw new RuntimeException('Invalid mail address'); return $v; }
function smtp_cmd($s, string $line, array $codes): string { fwrite($s, $line . "\r\n"); return smtp_expect($s, $codes); }
function smtp_expect($s, array $codes): string {
    $response = ''; do { $line = fgets($s, 1024); if ($line === false) throw new RuntimeException('SMTP read failed'); $response .= $line; } while (isset($line[3]) && $line[3] === '-');
    if (!in_array((int)substr($response, 0, 3), $codes, true)) throw new RuntimeException('SMTP rejected command'); return $response;
}
function rate_limit(string $bucket, int $limit): void {
    $dir = sys_get_temp_dir(); $ip = (string)($_SERVER['REMOTE_ADDR'] ?? 'unknown'); $file = $dir . '/antctrl-' . $bucket . '-' . hash('sha256', $ip) . '.json';
    foreach (glob($dir . '/antctrl-*.json') ?: [] as $old) if (@filemtime($old) < time() - 3600) @unlink($old);
    $h = @fopen($file, 'c+'); if (!$h || !flock($h, LOCK_EX)) { if ($h) fclose($h); fail(503, 'Die Schutzprüfung des Mailversands ist gerade nicht verfügbar.'); }
    rewind($h); $raw = stream_get_contents($h); $data = $raw === false ? [] : json_decode($raw, true);
    $now = time();
    $times = array_values(array_filter(is_array($data) ? $data : [], fn($t) => is_int($t) && $t > $now - 3600));
    if (count($times) >= $limit) { flock($h, LOCK_UN); fclose($h); fail(429, 'Zu viele Anfragen in kurzer Zeit. Bitte später erneut versuchen.'); }
    $times[] = $now; rewind($h); ftruncate($h, 0); $ok = fwrite($h, json_encode($times)) !== false && fflush($h); flock($h, LOCK_UN); fclose($h);
    if (!$ok) fail(503, 'Die Schutzprüfung des Mailversands ist gerade nicht verfügbar.');
}
function label(string $k): string { return ['category'=>'Art','subject'=>'Betreff','description'=>'Beschreibung','steps'=>'Schritte zur Wiederholung','expected'=>'Erwartetes Verhalten','since'=>'Seit wann / nach welcher Änderung','version'=>'Firmware-Version','name'=>'Name','contact'=>'Rückkontakt'][$k] ?? $k; }
function esc(string $s): string { return htmlspecialchars($s, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8'); }
function page(string $title, string $content): void {
    header('Content-Type: text/html; charset=UTF-8');
    echo '<!doctype html><html lang="de"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>' . esc($title) . '</title><style>body{font:16px system-ui,sans-serif;max-width:760px;margin:3rem auto;padding:0 1rem;background:#111820;color:#edf4fa}main{border:1px solid #385369;border-radius:12px;padding:1.5rem}dt{font-weight:700;color:#9edaff;margin-top:1rem}dd{margin:.25rem 0 0;white-space:normal}button{padding:.8rem 1.1rem;border:0;border-radius:8px;background:#2784c8;color:white;font-weight:700;cursor:pointer}.trap{position:absolute;left:-10000px}</style><main><h1>' . esc($title) . '</h1>' . $content . '</main></html>';
    exit;
}
function is_https(): bool { return (!empty($_SERVER['HTTPS']) && strtolower((string)$_SERVER['HTTPS']) !== 'off') || (string)($_SERVER['REQUEST_SCHEME'] ?? '') === 'https' || (string)($_SERVER['SERVER_PORT'] ?? '') === '443'; }
function health_result(bool $ready): void { http_response_code($ready ? 200 : 503); header('Content-Type: application/json; charset=utf-8'); echo json_encode(['ready'=>$ready], JSON_UNESCAPED_SLASHES); exit; }
function fail(int $status, string $message): void { global $isHealth; http_response_code($status); if ($isHealth) { header('Content-Type: application/json; charset=utf-8'); echo json_encode(['ready'=>false]); exit; } page('Meldeformular', '<p>' . esc($message) . '</p>'); }
