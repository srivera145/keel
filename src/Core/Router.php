<?php

namespace Keel\Core;

use Keel\Core\ErrorHandler;

class Router
{
    private static ?self $current = null;
    private array $routes = [];
    private array $groupMiddleware = [];
    private string $groupPrefix = '';

    public function __construct()
    {
        self::$current = $this;
    }

    public static function current(): ?self
    {
        return self::$current;
    }

    public function get(string $uri, callable|array $action, array $options = []): void
    {
        $this->addRoute('GET', $uri, $action, $options);
    }

    public function post(string $uri, callable|array $action): void
    {
        $this->addRoute('POST', $uri, $action);
    }

    public function put(string $uri, callable|array $action): void
    {
        $this->addRoute('PUT', $uri, $action);
    }

    public function delete(string $uri, callable|array $action): void
    {
        $this->addRoute('DELETE', $uri, $action);
    }

    public function group(array $options, \Closure $callback): void
    {
        $previousPrefix = $this->groupPrefix;
        $previousMiddleware = $this->groupMiddleware;

        $this->groupPrefix .= $options['prefix'] ?? '';
        $this->groupMiddleware = array_merge($this->groupMiddleware, $options['middleware'] ?? []);

        $callback($this);

        $this->groupPrefix = $previousPrefix;
        $this->groupMiddleware = $previousMiddleware;
    }

    private function addRoute(string $method, string $uri, callable|array $action, array $options = []): void
    {
        $uri = $this->groupPrefix . $uri;
        $pattern = preg_replace('/\{([a-zA-Z_]+)\}/', '(?P<$1>[^/]+)', $uri);
        $pattern = '#^' . rtrim($pattern, '/') . '/?$#';

        $this->routes[] = [
            'method' => $method,
            'uri' => $uri,
            'pattern' => $pattern,
            'action' => $action,
            'middleware' => $this->groupMiddleware,
            'sitemap' => (bool) ($options['sitemap'] ?? false),
            'options' => $options,
        ];
    }

    public function publicPages(): array
    {
        $pages = [];

        foreach ($this->routes as $route) {
            if ($route['method'] !== 'GET' || (bool) ($route['sitemap'] ?? false) !== true) {
                continue;
            }

            $pages[] = [
                'uri' => (string) $route['uri'],
                'options' => (array) ($route['options'] ?? []),
            ];
        }

        return $pages;
    }

    public function registeredRoutes(): array
    {
        return $this->routes;
    }

    public function dispatch(Request $request): mixed
    {
        $uri = rtrim($request->uri, '/') ?: '/';

        foreach ($this->routes as $route) {
            if ($route['method'] !== $request->method) {
                continue;
            }

            if (preg_match($route['pattern'], $uri, $matches)) {
                $params = array_filter($matches, fn ($k) => !is_int($k), ARRAY_FILTER_USE_KEY);

                $pipeline = array_reduce(
                    array_reverse($route['middleware']),
                    fn ($next, $middlewareClass) => fn ($req) => (new $middlewareClass())->handle($req, $next),
                    function ($req) use ($route, $params) {
                        return $this->callAction($route['action'], $req, $params);
                    }
                );

                return $pipeline($request);
            }
        }

        return ErrorHandler::render(404);
    }

    private function callAction(callable|array $action, Request $request, array $params): mixed
    {
        if (is_array($action)) {
            [$class, $method] = $action;
            $controller = new $class();
            return $controller->$method($request, ...array_values($params));
        }

        return $action($request, ...array_values($params));
    }
}
