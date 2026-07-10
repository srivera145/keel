<?php

namespace Keel\Core;

interface Middleware
{
    public function handle(Request $request, \Closure $next): mixed;
}
