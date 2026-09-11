<?php

namespace Keel\Core;

class Storage
{
    private const DISALLOWED_EXTENSIONS = [
        'php',
        'php3',
        'php4',
        'php5',
        'phtml',
        'phar',
        'exe',
        'dll',
        'sh',
        'bat',
        'cmd',
        'com',
        'msi',
        'js',
        'jar',
    ];

    private const MIME_EXTENSION_MAP = [
        'application/pdf' => ['pdf'],
        'image/jpeg' => ['jpg', 'jpeg'],
        'image/png' => ['png'],
        'image/heic' => ['heic'],
        'image/heif' => ['heic'],
    ];

    public static function putUploadedFile(array $file, string $directory, bool $public = false): array
    {
        self::ensureSupportedDisk();

        if (($file['error'] ?? UPLOAD_ERR_NO_FILE) !== UPLOAD_ERR_OK) {
            throw new \RuntimeException('Upload failed.');
        }

        $tmpName = (string) ($file['tmp_name'] ?? '');
        $originalName = (string) ($file['name'] ?? 'upload');
        $size = (int) ($file['size'] ?? 0);

        if ($tmpName === '' || !is_uploaded_file($tmpName)) {
            throw new \RuntimeException('Invalid uploaded file.');
        }

        $maxBytes = max(1, (int) Env::get('FILESYSTEM_MAX_UPLOAD_MB', 10)) * 1024 * 1024;
        if ($size <= 0 || $size > $maxBytes) {
            throw new \RuntimeException('File exceeds the configured upload limit.');
        }

        $extension = strtolower(pathinfo($originalName, PATHINFO_EXTENSION));
        if ($extension === '' || in_array($extension, self::DISALLOWED_EXTENSIONS, true)) {
            throw new \RuntimeException('That file type is not allowed.');
        }

        $allowedExtensions = self::allowedExtensions();
        if (!in_array($extension, $allowedExtensions, true)) {
            throw new \RuntimeException('That file extension is not permitted.');
        }

        $finfo = new \finfo(FILEINFO_MIME_TYPE);
        $mimeType = (string) $finfo->file($tmpName);

        if ($mimeType === '') {
            throw new \RuntimeException('Could not determine the uploaded file type.');
        }

        $allowedMimeExtensions = self::MIME_EXTENSION_MAP[$mimeType] ?? null;
        if ($allowedMimeExtensions === null || !in_array($extension, $allowedMimeExtensions, true)) {
            throw new \RuntimeException('The uploaded file type does not match its contents.');
        }

        $normalizedDirectory = self::normalizeRelativePath($directory);
        $relativeDirectory = $normalizedDirectory === '' ? '' : $normalizedDirectory . '/';
        $filename = bin2hex(random_bytes(16)) . '.' . $extension;
        $relativePath = $relativeDirectory . $filename;
        $absolutePath = self::absolutePath($relativePath, $public);
        $baseDirectory = dirname($absolutePath);

        if (!is_dir($baseDirectory) && !mkdir($baseDirectory, 0775, true) && !is_dir($baseDirectory)) {
            throw new \RuntimeException('Failed to prepare the upload directory.');
        }

        if (!move_uploaded_file($tmpName, $absolutePath)) {
            throw new \RuntimeException('Failed to store the uploaded file.');
        }

        return [
            'path' => $relativePath,
            'mime_type' => $mimeType,
            'size' => $size,
            'original_name' => $originalName,
        ];
    }

    public static function get(string $path, bool $public = false): string
    {
        self::ensureSupportedDisk();
        $absolutePath = self::absolutePath($path, $public);

        if (!is_file($absolutePath)) {
            throw new \RuntimeException('File not found.');
        }

        $contents = file_get_contents($absolutePath);

        if ($contents === false) {
            throw new \RuntimeException('Failed to read file.');
        }

        return $contents;
    }

    public static function delete(string $path, bool $public = false): bool
    {
        self::ensureSupportedDisk();
        $absolutePath = self::absolutePath($path, $public);

        if (!is_file($absolutePath)) {
            return false;
        }

        return unlink($absolutePath);
    }

    public static function url(string $path): string
    {
        self::ensureSupportedDisk();

        return '/uploads/' . self::normalizeRelativePath($path);
    }

    public static function absolutePath(string $path, bool $public = false): string
    {
        self::ensureSupportedDisk();

        $relativePath = self::normalizeRelativePath($path);
        $root = dirname(__DIR__, 2);
        $base = $public ? $root . '/public_html/uploads' : $root . '/storage/app';

        return $base . '/' . $relativePath;
    }

    public static function disk(): string
    {
        return strtolower(trim((string) Env::get('FILESYSTEM_DISK', 'local')));
    }

    private static function allowedExtensions(): array
    {
        $raw = strtolower((string) Env::get('FILESYSTEM_ALLOWED_EXTENSIONS', 'pdf,jpg,jpeg,png,heic'));
        $parts = array_filter(array_map('trim', explode(',', $raw)));

        return array_values(array_diff($parts, self::DISALLOWED_EXTENSIONS));
    }

    private static function ensureSupportedDisk(): void
    {
        if (self::disk() !== 'local') {
            throw new \RuntimeException('Only the local filesystem disk is supported right now.');
        }
    }

    private static function normalizeRelativePath(string $path): string
    {
        $segments = array_filter(explode('/', str_replace('\\', '/', $path)), static function ($segment): bool {
            return $segment !== '' && $segment !== '.' && $segment !== '..';
        });

        return implode('/', $segments);
    }
}