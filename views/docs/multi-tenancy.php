<div class="space-y-6 text-sm leading-7 text-[var(--color-text-muted)]">
    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Feature toggle</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>MULTI_TENANCY_ENABLED=false</code></pre>
        <p>When false, org-specific routes and middleware groups are not registered.</p>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Data model</h2>
        <ul class="list-disc space-y-1 pl-5">
            <li><code>organizations</code> table stores tenant records.</li>
            <li><code>users.organization_id</code> links users to one org.</li>
            <li><code>users.role</code> stores <code>owner</code>, <code>admin</code>, or <code>member</code>.</li>
            <li><code>organization_invites</code> stores hashed invite tokens with expiry and acceptance state.</li>
            <li><code>users.is_super_admin</code> gates platform-level pages.</li>
        </ul>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Flow</h2>
        <ol class="list-decimal space-y-1 pl-5">
            <li>User without an org is redirected to <code>/onboarding/organization</code> after login.</li>
            <li>Org owner/admin sends invite from <code>/settings/members</code>.</li>
            <li><code>OrganizationService::invite()</code> hashes token and queues an invite email job.</li>
            <li>Invitee opens <code>/invite/accept?email=...&amp;token=...</code>.</li>
            <li><code>OrganizationService::acceptInvite()</code> validates token, links/creates user, marks invite accepted.</li>
        </ol>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Role enforcement</h2>
        <ul class="list-disc space-y-1 pl-5">
            <li><code>RequireOrgAdminMiddleware</code> allows only <code>owner</code> and <code>admin</code> for org settings/members pages.</li>
            <li><code>RequireSuperAdminMiddleware</code> requires <code>is_super_admin = 1</code> for platform pages.</li>
        </ul>
    </section>
</div>
