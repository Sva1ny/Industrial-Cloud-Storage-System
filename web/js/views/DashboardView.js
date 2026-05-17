const DashboardView = {
    template: `
    <div class="dashboard">
      <!-- Top Bar -->
      <div class="top-bar">
        <div class="brand">☁️ CloudDisk</div>

        <div style="display:flex;gap:4px;">
          <button class="btn btn-sm" style="background:none;border:none;font-size:18px;cursor:pointer;" @click="store.selectedFiles.clear(); refresh();">↻</button>
        </div>

        <div class="user-info">
          <span>{{ api.getUsername() }}</span>
          <div class="avatar">👤</div>
          <button class="btn-link" @click="logout">Logout</button>
        </div>
      </div>

      <!-- Storage Bar -->
      <div class="storage-bar">
        <span>Storage</span>
        <div class="bar"><div class="fill" style="width:12%"></div></div>
        <span>12% used</span>
      </div>

      <div class="content">
        <!-- Sidebar -->
        <div class="sidebar">
          <div class="sidebar-item" :class="{ active: store.currentView === 'dashboard' }" @click="store.currentView = 'dashboard'; refresh();">📁 My Files</div>
          <div class="sidebar-item" :class="{ active: store.currentView === 'shares' }" @click="store.currentView = 'shares'; store.refreshShares();">🔗 Shares</div>
          <div class="sidebar-item" :class="{ active: store.currentView === 'trash' }" @click="store.currentView = 'trash'; store.refreshTrash();">🗑 Trash</div>
          <div style="border-top:1px solid var(--border-light);margin:8px 0;"></div>
          <div class="sidebar-item" @click="store.currentView = 'dashboard'; refresh();">⬆ Upload</div>
        </div>

        <!-- Main -->
        <div class="main-area">
          <!-- Files View -->
          <template v-if="store.currentView === 'dashboard'">
            <FileToolbar></FileToolbar>
            <FileTable></FileTable>
          </template>

          <!-- Trash View -->
          <template v-if="store.currentView === 'trash'">
            <div class="toolbar">
              <div class="breadcrumb"><span style="font-weight:600;">🗑 Trash</span></div>
              <button class="btn btn-danger btn-sm" @click="emptyTrash">Empty Trash</button>
              <button class="btn btn-secondary btn-sm" @click="store.refreshTrash()">↻ Refresh</button>
            </div>
            <div class="file-list-container">
              <table class="file-table" v-if="store.trashItems.length > 0">
                <thead>
                  <tr>
                    <th>Name</th>
                    <th style="width:100px">Size</th>
                    <th style="width:160px">Deleted</th>
                    <th style="width:160px">Expires</th>
                    <th style="width:140px">Actions</th>
                  </tr>
                </thead>
                <tbody>
                  <tr v-for="item in store.trashItems" :key="item.id">
                    <td><span class="file-icon">{{ item.is_folder ? '📁' : '📄' }}</span> {{ item.filename }}</td>
                    <td>{{ utils.formatSize(item.size) }}</td>
                    <td>{{ utils.formatDate(item.deleted_at) }}</td>
                    <td>{{ utils.formatDate(item.expire_at) }}</td>
                    <td>
                      <div class="actions">
                        <button class="btn btn-sm" style="background:none;border:none;color:var(--primary);font-size:12px;cursor:pointer;" @click="restoreItem(item.id)">Restore</button>
                        <button class="btn btn-sm" style="background:none;border:none;color:var(--danger);font-size:12px;cursor:pointer;" @click="deleteForever(item.id)">Delete</button>
                      </div>
                    </td>
                  </tr>
                </tbody>
              </table>
              <div v-else class="empty-state"><div class="icon">🗑</div><p>Trash is empty.</p></div>
            </div>
          </template>

          <!-- Shares View -->
          <template v-if="store.currentView === 'shares'">
            <div class="toolbar">
              <div class="breadcrumb"><span style="font-weight:600;">🔗 Shares</span></div>
              <button class="btn btn-secondary btn-sm" @click="store.refreshShares()">↻ Refresh</button>
            </div>
            <div class="file-list-container">
              <table class="file-table" v-if="store.shares.length > 0">
                <thead>
                  <tr>
                    <th>File</th>
                    <th>Type</th>
                    <th style="width:160px">Created</th>
                    <th style="width:160px">Expires</th>
                    <th style="width:80px">Downloads</th>
                    <th style="width:80px">Actions</th>
                  </tr>
                </thead>
                <tbody>
                  <tr v-for="s in store.shares" :key="s.id">
                    <td>{{ s.filename }}</td>
                    <td>{{ s.share_type === 0 ? 'View' : 'Download' }}</td>
                    <td>{{ utils.formatDate(s.created_at) }}</td>
                    <td>{{ utils.formatDate(s.expire_at) }}</td>
                    <td>{{ s.download_count }}</td>
                    <td>
                      <button class="btn btn-sm" style="background:none;border:none;color:var(--danger);font-size:12px;cursor:pointer;" @click="deleteShare(s.id)">Delete</button>
                    </td>
                  </tr>
                </tbody>
              </table>
              <div v-else class="empty-state"><div class="icon">🔗</div><p>No shares yet.</p></div>
            </div>
          </template>
        </div>
      </div>
    </div>
    `,
    data() { return { store, api, utils }; },
    methods: {
        async refresh() { await store.refreshFiles(); },
        logout() {
            api.clearAuth();
            store.currentView = 'signin';
            store.files = [];
            store.selectedFiles.clear();
        },
        async emptyTrash() {
            if (!confirm('Permanently delete all trashed files?')) return;
            await api.emptyTrash();
            await store.refreshTrash();
        },
        async restoreItem(id) {
            await api.restoreTrash([id]);
            await store.refreshTrash();
        },
        async deleteForever(id) {
            if (!confirm('Permanently delete this file?')) return;
            await api.deleteForever([id]);
            await store.refreshTrash();
        },
        async deleteShare(id) {
            if (!confirm('Delete this share link?')) return;
            await api.deleteShare(id);
            await store.refreshShares();
        }
    },
    created() {
        // Keyboard shortcuts
        document.addEventListener('keydown', this.onKeyDown);
    },
    beforeUnmount() {
        document.removeEventListener('keydown', this.onKeyDown);
    },
    created() {
        document.addEventListener('keydown', this.onKeyDown);
    },
    beforeUnmount() {
        document.removeEventListener('keydown', this.onKeyDown);
    },
    mounted() {
        store.refreshFiles();
    },
    onKeyDown(e) {
        // Ctrl+A: select all
        if (e.ctrlKey && e.key === 'a' && store.currentView === 'dashboard') {
            e.preventDefault();
            store.selectAll();
        }
        // Delete: batch delete selected
        if (e.key === 'Delete' && store.selectedFiles.size > 0) {
            e.preventDefault();
            if (confirm('Delete ' + store.selectedFiles.size + ' item(s)?')) {
                const ids = [...store.selectedFiles];
                api.batchDelete(ids).then(res => {
                    if (res.success) {
                        store.selectedFiles.clear();
                        store.refreshFiles();
                    }
                });
            }
        }
        // F2: rename first selected
        if (e.key === 'F2' && store.selectedFiles.size === 1) {
            e.preventDefault();
            const file = store.files.find(f => store.selectedFiles.has(f.id));
            if (file && !file.is_folder) {
                const newName = prompt('Rename:', file.filename);
                if (newName && newName !== file.filename) {
                    api.renameFile(file.id, newName).then(res => {
                        if (res.success) store.refreshFiles();
                    });
                }
            }
        }
    }
};
