const FileTable = {
    template: `
    <div class="file-list-container">
      <!-- Action bar -->
      <div style="display:flex;gap:8px;margin-bottom:8px;align-items:center;min-height:32px;">
        <label style="display:flex;align-items:center;gap:4px;font-size:13px;cursor:pointer;">
          <input type="checkbox" :checked="allSelected" @change="store.selectAll()">
          Select All
        </label>
        <template v-if="store.selectedFiles.size > 0">
          <button class="btn btn-danger btn-sm" @click="batchDelete">Delete ({{ store.selectedFiles.size }})</button>
          <button class="btn btn-secondary btn-sm" @click="showMove = true">Move</button>
          <button class="btn btn-secondary btn-sm" @click="batchCopy">Copy</button>
          <span style="font-size:12px;color:var(--text-secondary);">{{ store.selectedFiles.size }} selected</span>
        </template>
      </div>

      <!-- Grid view -->
      <div v-if="store.viewMode === 'grid' && store.files.length > 0" style="display:grid;grid-template-columns:repeat(auto-fill,140px);gap:12px;padding:8px 0;">
        <div v-for="f in store.files" :key="f.id"
          @dblclick="openItem(f)"
          @click.ctrl="store.toggleSelect(f.id)"
          :class="{ selected: store.selectedFiles.has(f.id) }"
          style="display:flex;flex-direction:column;align-items:center;padding:16px 8px;border-radius:8px;cursor:pointer;transition:background 0.15s;border:1px solid transparent;"
          @mouseenter="$el.style.background='#FAFAFB'"
          @mouseleave="$el.style.background=''">
          <div style="font-size:40px;margin-bottom:8px;">{{ utils.getFileIcon(f) }}</div>
          <div style="font-size:12px;text-align:center;word-break:break-all;line-height:1.2;">{{ f.filename }}</div>
          <div style="font-size:11px;color:var(--text-secondary);margin-top:4px;">{{ utils.formatSize(f.size) }}</div>
        </div>
      </div>

      <!-- List view -->
      <table v-else-if="store.files.length > 0" class="file-table">
        <thead>
          <tr>
            <th style="width:32px"></th>
            <th class="sortable" @click="sortBy('filename')">Name</th>
            <th class="sortable" @click="sortBy('size')" style="width:100px">Size</th>
            <th class="sortable" @click="sortBy('updated_at')" style="width:160px">Modified</th>
            <th style="width:160px">Actions</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="f in store.files" :key="f.id"
            :class="{ selected: store.selectedFiles.has(f.id) }"
            @dblclick="openItem(f)">
            <td><input type="checkbox" :checked="store.selectedFiles.has(f.id)" @change="store.toggleSelect(f.id)"></td>
            <td>
              <div class="file-name" @click="openItem(f)">
                <span class="file-icon">{{ utils.getFileIcon(f) }}</span>
                <span>{{ f.filename }}</span>
              </div>
            </td>
            <td>{{ utils.formatSize(f.size) }}</td>
            <td>{{ utils.formatDate(f.updated_at) }}</td>
            <td>
              <div class="actions">
                <button class="btn btn-sm" style="background:none;border:none;color:var(--primary);font-size:12px;cursor:pointer;" @click.stop="renameFile(f)">Rename</button>
                <button class="btn btn-sm" style="background:none;border:none;color:var(--primary);font-size:12px;cursor:pointer;" @click.stop="showHistory(f)" v-if="!f.is_folder">History</button>
                <button class="btn btn-sm" style="background:none;border:none;color:var(--primary);font-size:12px;cursor:pointer;" @click.stop="showShareDialog(f)">Share</button>
                <button class="btn btn-sm" style="background:none;border:none;color:#34C759;font-size:12px;cursor:pointer;" @click.stop="downloadFolder(f)" v-if="f.is_folder">Download</button>
                <button class="btn btn-sm" style="background:none;border:none;color:var(--danger);font-size:12px;cursor:pointer;" @click.stop="deleteFile(f)">Delete</button>
              </div>
            </td>
          </tr>
        </tbody>
      </table>

      <!-- Empty state -->
      <div v-else class="empty-state">
        <div class="icon">📂</div>
        <p v-if="store.isLoading">Loading...</p>
        <p v-else>No files yet. Upload your first file to get started.</p>
      </div>

      <!-- Move Dialog -->
      <div class="modal-overlay" v-if="showMove" @click.self="showMove = false">
        <div class="modal">
          <h2>Move {{ store.selectedFiles.size }} item(s)</h2>
          <p style="font-size:13px;color:var(--text-secondary);margin-bottom:16px;">Items will be moved to the selected folder below.</p>
          <div class="form-group">
            <select v-model="moveTarget" style="width:100%;padding:10px;border:1px solid var(--border-light);border-radius:8px;font-size:14px;">
              <option :value="0">Root</option>
              <option v-for="f in store.files.filter(f => f.is_folder)" :key="f.id" :value="f.id">{{ f.filename }}</option>
            </select>
          </div>
          <div style="display:flex;gap:8px;justify-content:flex-end;">
            <button class="btn btn-secondary" @click="showMove = false">Cancel</button>
            <button class="btn btn-primary" style="width:auto" @click="doMove">Move</button>
          </div>
        </div>
      </div>

      <!-- Share Dialog -->
      <div class="modal-overlay" v-if="shareFile" @click.self="shareFile = null">
        <div class="modal">
          <h2>Share: {{ shareFile.filename }}</h2>
          <div class="form-group">
            <label style="font-size:13px;display:block;margin-bottom:4px;">Expires in (days):</label>
            <input v-model.number="shareExpire" type="number" min="1" max="365" style="width:100%;padding:10px;border:1px solid var(--border-light);border-radius:8px;font-size:14px;">
          </div>
          <div class="form-group">
            <label style="font-size:13px;display:block;margin-bottom:4px;">Password (optional):</label>
            <input v-model="sharePassword" type="text" placeholder="Leave blank for no password" style="width:100%;padding:10px;border:1px solid var(--border-light);border-radius:8px;font-size:14px;">
          </div>
          <div v-if="shareResult" style="margin-bottom:16px;padding:12px;background:#F5F5F7;border-radius:8px;font-size:12px;word-break:break-all;">
            <strong>Share URL:</strong><br>
            <a :href="shareResult" target="_blank">{{ shareResult }}</a>
          </div>
          <div style="display:flex;gap:8px;justify-content:flex-end;">
            <button class="btn btn-secondary" @click="shareFile = null">Close</button>
            <button class="btn btn-primary" style="width:auto" @click="doShare">Create Link</button>
          </div>
        </div>
      </div>

      <!-- Rename Dialog -->
      <div class="modal-overlay" v-if="renameTarget" @click.self="renameTarget = null">
        <div class="modal">
          <h2>Rename</h2>
          <div class="form-group">
            <input v-model="renameValue" placeholder="New name" @keyup.enter="doRename" autofocus style="width:100%;padding:10px;border:1px solid var(--border-light);border-radius:8px;font-size:14px;">
          </div>
          <div style="display:flex;gap:8px;justify-content:flex-end;">
            <button class="btn btn-secondary" @click="renameTarget = null">Cancel</button>
            <button class="btn btn-primary" style="width:auto" @click="doRename">Rename</button>
          </div>
        </div>
      </div>

      <!-- Version History Dialog -->
      <div class="modal-overlay" v-if="historyFile" @click.self="historyFile = null">
        <div class="modal" style="width:560px;">
          <h2>📋 Version History: {{ historyFile.filename }}</h2>
          <div v-if="historyLoading" style="text-align:center;padding:20px;color:var(--text-secondary);">Loading...</div>
          <div v-else style="max-height:400px;overflow-y:auto;">
            <div v-for="(v, i) in historyVersions" :key="i" style="display:flex;align-items:center;padding:10px 0;border-bottom:1px solid var(--border-light);gap:12px;">
              <div style="flex:1;min-width:0;">
                <div style="font-size:13px;font-weight:600;">{{ v.is_current ? 'Current' : 'Version ' + v.version }}</div>
                <div style="font-size:12px;color:var(--text-secondary);">{{ utils.formatSize(v.size) }} · {{ utils.formatDate(v.created_at) }}</div>
              </div>
              <button class="btn btn-sm" style="background:none;border:1px solid var(--border);border-radius:4px;cursor:pointer;font-size:12px;padding:4px 12px;" @click="downloadVersion(v, historyFile.filename)">Download</button>
            </div>
            <div v-if="!historyVersions.length" style="text-align:center;padding:20px;color:var(--text-secondary);">No history available.</div>
          </div>
          <div style="display:flex;gap:8px;justify-content:flex-end;margin-top:16px;">
            <button class="btn btn-secondary" @click="historyFile = null">Close</button>
          </div>
        </div>
      </div>
    </div>
    `,
    data() {
        return {
            store, utils,
            showMove: false, moveTarget: 0,
            shareFile: null, sharePassword: '', shareExpire: 7, shareResult: '',
            renameTarget: null, renameValue: '',
            historyFile: null, historyVersions: [], historyLoading: false
        };
    },
    computed: {
        allSelected() {
            return store.files.length > 0 && store.selectedFiles.size === store.files.length;
        }
    },
    methods: {
        openItem(f) {
            if (f.is_folder) {
                store.navigateToFolder(f.id, f.filename);
            } else {
                // Download via legacy endpoint
                const url = '/file/download?username=' + encodeURIComponent(api.getUsername())
                    + '&token=' + encodeURIComponent(api.getToken())
                    + '&filename=' + encodeURIComponent(f.filename)
                    + '&filehash=' + f.hashcode;
                window.open(url, '_blank');
            }
        },
        async downloadFolder(f) {
            if (!f.is_folder) return;
            // Trigger ZIP download via temp iframe
            const form = document.createElement('form');
            form.method = 'POST';
            form.action = '/api/file/download_zip';
            form.target = '_blank';
            const input = document.createElement('input');
            input.type = 'hidden';
            input.name = 'data';
            input.value = JSON.stringify({ folder_id: f.id });
            form.appendChild(input);
            // Add auth header via fetch-based approach
            const url = '/api/file/download_zip';
            const res = await fetch(url, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Authorization': 'Bearer ' + api.getToken()
                },
                body: JSON.stringify({ folder_id: f.id })
            });
            if (!res.ok) { alert('Download failed'); return; }
            // Create blob and trigger download
            const blob = await res.blob();
            const blobUrl = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = blobUrl;
            a.download = f.filename + '.zip';
            a.click();
            URL.revokeObjectURL(blobUrl);
        },
        sortBy(col) {
            if (store.sortBy === col) {
                store.sortOrder = store.sortOrder === 'asc' ? 'desc' : 'asc';
            } else {
                store.sortBy = col;
                store.sortOrder = 'asc';
            }
            store.refreshFiles();
        },
        async batchDelete() {
            if (!confirm('Delete ' + store.selectedFiles.size + ' item(s)?')) return;
            const ids = [...store.selectedFiles];
            const res = await api.batchDelete(ids);
            if (res.success) {
                store.selectedFiles.clear();
                await store.refreshFiles();
            }
        },
        async doMove() {
            const ids = [...store.selectedFiles];
            const res = await api.moveFiles(ids, this.moveTarget);
            if (res.success) {
                this.showMove = false;
                store.selectedFiles.clear();
                await store.refreshFiles();
            }
        },
        async batchCopy() {
            const ids = [...store.selectedFiles];
            const target = parseInt(prompt('Target folder ID (0 for root):') || '0');
            if (isNaN(target)) return;
            const res = await api.copyFiles(ids, target);
            if (res.success) {
                store.selectedFiles.clear();
                await store.refreshFiles();
            }
        },
        async deleteFile(f) {
            if (!confirm('Delete "' + f.filename + '"?')) return;
            const res = await api.batchDelete([f.id]);
            if (res.success) await store.refreshFiles();
        },
        renameFile(f) {
            this.renameTarget = f;
            this.renameValue = f.filename;
        },
        async doRename() {
            if (!this.renameValue) return;
            const res = await api.renameFile(this.renameTarget.id, this.renameValue);
            if (res.success) {
                this.renameTarget = null;
                await store.refreshFiles();
            }
        },
        showShareDialog(f) {
            this.shareFile = f;
            this.sharePassword = '';
            this.shareExpire = 7;
            this.shareResult = '';
        },
        async doShare() {
            const res = await api.createShare(this.shareFile.id, 0, this.sharePassword, this.shareExpire);
            if (res.success) {
                this.shareResult = window.location.origin + res.data.share_url;
            }
        },
        async showHistory(f) {
            this.historyFile = f;
            this.historyVersions = [];
            this.historyLoading = true;
            const res = await api.getFileHistory(f.id);
            if (res.success) this.historyVersions = res.data.versions || [];
            this.historyLoading = false;
        },
        async downloadVersion(v, filename) {
            const url = '/file/download?username=' + encodeURIComponent(api.getUsername())
                + '&token=' + encodeURIComponent(api.getToken())
                + '&filename=' + encodeURIComponent(filename + (v.is_current ? '' : '.v' + v.version))
                + '&filehash=' + v.hashcode;
            window.open(url, '_blank');
        }
    }
};
