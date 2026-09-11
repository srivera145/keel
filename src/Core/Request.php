<?php

namespace Keel\Core;

class Request
{
    public string $method;
    public string $uri;
    public array $query;
    public array $body;
    public array $headers;
    private string $rawBody;

    public function __construct()
    {
        $this->method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
        $this->uri = parse_url($_SERVER['REQUEST_URI'] ?? '/', PHP_URL_PATH) ?: '/';
        $this->query = $_GET;
        $this->headers = function_exists('getallheaders') ? getallheaders() : [];
        $rawBody = $_SERVER['KEEL_RAW_BODY'] ?? file_get_contents('php://input');
        $this->rawBody = is_string($rawBody) ? $rawBody : '';
        $this->body = $this->parseBody();
    }

    private function parseBody(): array
    {
        $contentType = $this->headers['Content-Type'] ?? $_SERVER['CONTENT_TYPE'] ?? '';

        if (str_contains($contentType, 'application/json')) {
            return json_decode($this->rawBody, true) ?? [];
        }

        return $_POST;
    }

    public function input(string $key, mixed $default = null): mixed
    {
        return $this->body[$key] ?? $this->query[$key] ?? $default;
    }

    public function all(): array
    {
        return array_merge($this->query, $this->body);
    }

    public function isJson(): bool
    {
        $contentType = $this->headers['Content-Type'] ?? $_SERVER['CONTENT_TYPE'] ?? '';
        return str_contains($contentType, 'application/json');
    }

    public function wantsJson(): bool
    {
        $accept = $this->headers['Accept'] ?? $_SERVER['HTTP_ACCEPT'] ?? '';
        return str_contains($accept, 'application/json') || $this->isJson();
    }

    public function rawBody(): string
    {
        return $this->rawBody;
    }
}
