const FileToolbar = {
    template: `
    <div class="toolbar">
      <div class="breadcrumb">
        <a @click="navigateRoot">CloudDisk</a>
        <span v-for="(folder, i) in store.currentPath" :key="folder.id">
          <span>/</span>
          <a @click="navigateTo(folder, i)" :class="{ active: i === store.currentPath.length - 1 }">{{ folder.name }}</a>
        </span>
      </div>

      <div class="search-box">
        <input v-model="searchText" placeholder="Search files..." @keyup.enter="doSearch" @keyup.escape="clearSearch">
      </div>

      <button class="btn btn-primary btn-sm" @click="showNewFolder = true">+ New Folder</button>
      <button class="btn btn-secondary btn-sm" @click="triggerUpload">⬆ Upload</button>
      <input ref="fileInput" type="file" multiple style="display:none" @change="uploadFiles">
      <input ref="folderInput" type="file" webkitdirectory style="display:none" @change="uploadFolder">

      <select v-model="store.sortBy" @change="refresh" class="btn btn-sm" style="background:var(--hover);border:1px solid var(--border-light);border-radius:6px;padding:6px 10px;font-size:12px;cursor:pointer;outline:none;">
        <option value="filename">Name</option>
        <option value="size">Size</option>
        <option value="updated_at">Modified</option>
      </select>
      <button class="btn btn-sm" style="background:none;border:none;font-size:16px;cursor:pointer;" @click="toggleView">{{ store.viewMode === 'list' ? '⊞' : '☰' }}</button>

      <!-- New Folder Modal -->
      <div class="modal-overlay" v-if="showNewFolder" @click.self="showNewFolder = false">
        <div class="modal">
          <h2>New Folder</h2>
          <div class="form-group">
            <input v-model="newFolderName" placeholder="Folder name" @keyup.enter="createFolder" autofocus>
          </div>
          <div style="display:flex;gap:8px;justify-content:flex-end;">
            <button class="btn btn-secondary" @click="showNewFolder = false">Cancel</button>
            <button class="btn btn-primary" style="width:auto" @click="createFolder">Create</button>
          </div>
        </div>
      </div>

      <!-- Upload Progress -->
      <div class="transfer-queue" v-if="store.transfers.length > 0">
        <div class="header">
          <span>Transfers</span>
          <button class="btn-link" @click="clearTransfers">Clear</button>
        </div>
        <div class="list">
          <div class="transfer-item" v-for="t in store.transfers" :key="t.id">
            <div class="name">
              <span>{{ t.name }}</span>
              <span>{{ t.status === 'active' ? t.progress + '%' : t.status }}</span>
            </div>
            <div class="bar">
              <div class="fill" :style="{ width: t.progress + '%' }" :class="t.status"></div>
            </div>
          </div>
        </div>
      </div>
    </div>
    `,
    data() {
        return { store, showNewFolder: false, newFolderName: '', searchText: '' };
    },
    methods: {
        navigateRoot() { store.navigateToPath(-1); },
        navigateTo(folder, index) { store.navigateToPath(index); },
        async refresh() { await store.refreshFiles(); },
        toggleView() { store.viewMode = store.viewMode === 'list' ? 'grid' : 'list'; },
        async createFolder() {
            if (!this.newFolderName) return;
            const res = await api.createFolder(store.currentFolderId, this.newFolderName);
            if (res.success) {
                this.showNewFolder = false;
                this.newFolderName = '';
                await store.refreshFiles();
            }
        },
        triggerUpload() { this.$refs.fileInput.click(); },
        async uploadFiles(e) {
            const files = e.target.files;
            for (const file of files) {
                const tid = store.addTransfer(file.name, 'upload');
                const fd = new FormData();
                fd.append('file', file);
                // username+token as query params (server reads from URL, not form body)
                const url = '/file/upload?username=' + encodeURIComponent(api.getUsername())
                    + '&token=' + encodeURIComponent(api.getToken());
                try {
                    await api.upload(url, fd, (sent, total) => {
                        store.updateTransfer(tid, Math.round(sent / total * 100));
                    });
                    store.completeTransfer(tid, 'done');
                } catch {
                    store.completeTransfer(tid, 'error');
                }
            }
            e.target.value = '';
            await store.refreshFiles();
        },
        uploadFolder(e) {
            // Folder upload via webkitdirectory - same handler
            this.uploadFiles(e);
        },
        async doSearch() {
            if (!this.searchText.trim()) return;
            store.searchQuery = this.searchText;
            const res = await api.searchFiles(this.searchText);
            if (res.success) store.files = res.data.files || [];
        },
        clearSearch() {
            this.searchText = '';
            store.searchQuery = '';
            store.refreshFiles();
        },
        clearTransfers() {
            store.transfers = [];
        }
    }
};
