<?php

namespace Keel\App\Controllers;

use Keel\App\Services\OrganizationService;
use Keel\Core\Controller;
use Keel\Core\Database;
use Keel\Core\Request;

class SuperAdminController extends Controller
{
    public function index(Request $request): void
    {
        $this->view('super-admin.organizations', [
            'title' => 'Super Admin',
            'organizations' => (new OrganizationService())->allOrganizations(),
            'selectedOrganization' => null,
            'selectedMembers' => [],
        ]);
    }

    public function showOrganization(Request $request, string $id): void
    {
        $organizationId = (int) $id;
        $service = new OrganizationService();

        $statement = Database::connection()->prepare('SELECT * FROM organizations WHERE id = ? LIMIT 1');
        $statement->execute([$organizationId]);
        $organization = $statement->fetch();

        if (!$organization) {
            $this->redirect('/super-admin/organizations');
        }

        $this->view('super-admin.organizations', [
            'title' => 'Super Admin',
            'organizations' => $service->allOrganizations(),
            'selectedOrganization' => $organization,
            'selectedMembers' => $service->members($organizationId),
        ]);
    }
}