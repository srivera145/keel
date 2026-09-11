<?php

declare(strict_types=1);

// Generates light brand variants for dark surfaces by remapping navy pixels to near-white.
// Orange accent and alpha transparency are preserved.

$options = getopt('', ['source:', 'target:', 'served-target:']);

$projectRoot = dirname(__DIR__);
$source = isset($options['source'])
    ? (string) $options['source']
    : $projectRoot . '/resources/images/brand/keel-icon.png';
$target = isset($options['target'])
    ? (string) $options['target']
    : $projectRoot . '/resources/images/brand/keel-icon-light.png';
$servedTarget = isset($options['served-target'])
    ? (string) $options['served-target']
    : $projectRoot . '/public_html/images/brand/keel-icon-light.png';

$source = normalizePath($source, $projectRoot);
$target = normalizePath($target, $projectRoot);
$servedTarget = normalizePath($servedTarget, $projectRoot);

if (!is_file($source)) {
    fwrite(STDERR, "Source icon not found: {$source}\n");
    exit(1);
}

if (extension_loaded('imagick')) {
    $image = new Imagick($source);
    $image->setImageAlphaChannel(Imagick::ALPHACHANNEL_SET);
    $image->setImageFormat('png');

    // Replace brand navy with near-white while keeping anti-aliased alpha edges intact.
    $image->opaquePaintImage('rgb(2,19,50)', 'rgb(248,250,252)', 0.15 * Imagick::getQuantum(), false);
    $image->writeImage($target);
} elseif (extension_loaded('gd')) {
    $image = imagecreatefrompng($source);
    if ($image === false) {
        fwrite(STDERR, "Failed to load PNG via GD.\n");
        exit(1);
    }

    imagesavealpha($image, true);
    imagealphablending($image, false);

    $width = imagesx($image);
    $height = imagesy($image);

    for ($y = 0; $y < $height; $y++) {
        for ($x = 0; $x < $width; $x++) {
            $rgba = imagecolorat($image, $x, $y);
            $a = ($rgba & 0x7F000000) >> 24;
            $r = ($rgba >> 16) & 0xFF;
            $g = ($rgba >> 8) & 0xFF;
            $b = $rgba & 0xFF;

            if ($a === 127) {
                continue;
            }

            $distance = sqrt((($r - 2) ** 2) + (($g - 19) ** 2) + (($b - 50) ** 2));
            if ($distance <= 45) {
                $newColor = imagecolorallocatealpha($image, 248, 250, 252, $a);
                imagesetpixel($image, $x, $y, $newColor);
            }
        }
    }

    imagepng($image, $target, 9);
    imagedestroy($image);
} else {
    // Neither GD nor Imagick extension is available in this environment, so use ImageMagick CLI.
    $magick = trim((string) shell_exec('where magick 2>NUL'));
    if ($magick === '') {
        fwrite(STDERR, "Neither PHP Imagick/GD nor ImageMagick CLI is available.\n");
        exit(1);
    }

    $sourceArg = escapeshellarg($source);
    $targetArg = escapeshellarg($target);
    $command = 'magick ' . $sourceArg
        . ' -alpha on -fuzz 15% -fill "#F8FAFC" -opaque "#021332" -strip'
        . ' -define png:compression-level=9 -define png:compression-filter=5 -define png:compression-strategy=1 '
        . $targetArg;

    exec($command, $output, $code);
    if ($code !== 0 || !is_file($target)) {
        fwrite(STDERR, "ImageMagick conversion failed.\n");
        exit(1);
    }
}

$servedDirectory = dirname($servedTarget);
if (!is_dir($servedDirectory) && !mkdir($servedDirectory, 0775, true) && !is_dir($servedDirectory)) {
    fwrite(STDERR, "Failed to create served icon directory.\n");
    exit(1);
}

if (!copy($target, $servedTarget)) {
    fwrite(STDERR, "Failed to copy light icon to public_html/images/brand.\n");
    exit(1);
}

fwrite(STDOUT, "Generated {$target}\n");
fwrite(STDOUT, "Copied {$servedTarget}\n");

function normalizePath(string $value, string $projectRoot): string
{
    if ($value === '') {
        return $value;
    }

    $looksAbsoluteUnix = str_starts_with($value, '/');
    $looksAbsoluteWindows = (bool) preg_match('/^[A-Za-z]:[\\\\\/]/', $value);
    if ($looksAbsoluteUnix || $looksAbsoluteWindows) {
        return $value;
    }

    return rtrim($projectRoot, '/\\') . DIRECTORY_SEPARATOR . ltrim($value, '/\\');
}
