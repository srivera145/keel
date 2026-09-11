<div class="prose">
    <h2>Feature toggle</h2>
    <pre><code>MULTI_TENANCY_ENABLED=false</code></pre>
    <p>When false, org-specific routes and middleware groups are not registered.</p>

    <h2>Data model</h2>
    <ul>
        <li><code>organizations</code> table stores tenant records.</li>
        <li><code>users.organization_id</code> links users to one org.</li>
        <li><code>users.role</code> stores <code>owner</code>, <code>admin</code>, or <code>member</code>.</li>
        <li><code>organization_invites</code> stores hashed invite tokens with expiry and acceptance state.</li>
        <li><code>users.is_super_admin</code> gates platform-level pages.</li>
    </ul>

    <h2>Flow</h2>
    <ol>
        <li>User without an org is redirected to <code>/onboarding/organization</code> after login.</li>
        <li>Org owner/admin sends invite from <code>/settings/members</code>.</li>
        <li><code>OrganizationService::invite()</code> hashes token and queues an invite email job.</li>
        <li>Invitee opens <code>/invite/accept?email=...&amp;token=...</code>.</li>
        <li><code>OrganizationService::acceptInvite()</code> validates token, links/creates user, marks invite accepted.</li>
    </ol>

    <h2>Role enforcement</h2>
    <ul>
        <li><code>RequireOrgAdminMiddleware</code> allows only <code>owner</code> and <code>admin</code> for org settings/members pages.</li>
        <li><code>RequireSuperAdminMiddleware</code> requires <code>is_super_admin = 1</code> for platform pages.</li>
    </ul>
</div>
