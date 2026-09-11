<?php

namespace Keel\App\Controllers;

use Keel\App\Models\File;
use Keel\Core\Activity;
use Keel\Core\Auth;
use Keel\Core\Controller;
use Keel\Core\Request;
use Keel\Core\Response;
use Keel\Core\Storage;

class FileController extends Controller
{
    public function store(Request $request): never
    {
        try {
            $upload = $_FILES['file'] ?? null;

            if (!is_array($upload)) {
                $this->json(['success' => false, 'message' => 'No file was uploaded.'], 422);
            }

            $directory = trim((string) $request->input('directory', 'uploads'));
            $isPublic = filter_var($request->input('is_public', false), FILTER_VALIDATE_BOOL);
            $stored = Storage::putUploadedFile($upload, $directory, $isPublic);
            $userId = (int) Auth::id();

            $id = File::create([
                'user_id' => $userId,
                'disk' => Storage::disk(),
                'path' => $stored['path'],
                'original_name' => $stored['original_name'],
                'mime_type' => $stored['mime_type'],
                'size_bytes' => $stored['size'],
                'is_public' => $isPublic ? 1 : 0,
            ]);

            Activity::log('file.uploaded', 'File', $id, [
                'filename' => (string) ($stored['original_name'] ?? ''),
            ]);

            $file = File::find($id);

            $this->json([
                'success' => true,
                'file' => [
                    'id' => $id,
                    'path' => $file['path'],
                    'mime_type' => $file['mime_type'],
                    'size_bytes' => (int) $file['size_bytes'],
                    'original_name' => $file['original_name'],
                    'url' => (int) $file['is_public'] === 1 ? Storage::url($file['path']) : '/files/' . $id,
                ],
            ], 201);
        } catch (\RuntimeException $exception) {
            $this->json(['success' => false, 'message' => $exception->getMessage()], 422);
        }
    }

    public function show(Request $request, string $id): never
    {
        $file = File::find((int) $id);

        if (!$file) {
            Response::abort(404, 'File not found');
        }

        if ((int) $file['user_id'] !== (int) Auth::id()) {
            Response::abort(403, 'Forbidden');
        }

        if ((int) $file['is_public'] === 1) {
            // Public redirects are intentionally not logged by default.
            Response::redirect(Storage::url($file['path']));
        }

        Activity::log('file.viewed', 'File', (int) $id);

        $absolutePath = Storage::absolutePath($file['path']);
        if (!is_file($absolutePath)) {
            Response::abort(404, 'File not found');
        }

        header('Content-Type: ' . $file['mime_type']);
        header('Content-Length: ' . filesize($absolutePath));
        header('Content-Disposition: inline; filename="' . addslashes((string) $file['original_name']) . '"');
        readfile($absolutePath);
        exit;
    }
}