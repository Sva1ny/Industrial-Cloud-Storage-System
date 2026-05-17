// CloudDisk API Client
const api = {
    getToken() {
        return localStorage.getItem('clouddisk_token') || '';
    },
    setToken(token) {
        localStorage.setItem('clouddisk_token', token);
    },
    getUsername() {
        return localStorage.getItem('clouddisk_username') || '';
    },
    setUsername(name) {
        localStorage.setItem('clouddisk_username', name);
    },
    clearAuth() {
        localStorage.removeItem('clouddisk_token');
        localStorage.removeItem('clouddisk_username');
    },

    async request(method, path, data) {
        const token = this.getToken();
        const headers = { 'Content-Type': 'application/json' };
        if (token) headers['Authorization'] = 'Bearer ' + token;

        const opts = { method, headers };
        if (data && method !== 'GET') opts.body = JSON.stringify(data);

        const res = await fetch(path, opts);
        if (res.status === 401) {
            this.clearAuth();
            window.location.hash = '#signin';
            throw new Error('unauthorized');
        }
        return res.json();
    },

    async get(path, params) {
        const qs = params ? '?' + new URLSearchParams(params).toString() : '';
        return this.request('GET', path + qs);
    },

    async post(path, data) {
        return this.request('POST', path, data);
    },

    // Upload with progress via XMLHttpRequest
    upload(path, formData, onProgress) {
        return new Promise((resolve, reject) => {
            const xhr = new XMLHttpRequest();
            xhr.open('POST', path);
            xhr.setRequestHeader('Authorization', 'Bearer ' + this.getToken());

            xhr.upload.onprogress = (e) => {
                if (onProgress && e.lengthComputable)
                    onProgress(e.loaded, e.total);
            };
            xhr.onload = () => {
                try { resolve(JSON.parse(xhr.responseText)); }
                catch { reject(new Error('parse error')); }
            };
            xhr.onerror = () => reject(new Error('network error'));
            xhr.send(formData);
        });
    },

    // Auth
    async signin(username, password) {
        const params = new URLSearchParams({ username, password });
        const res = await fetch('/user/signin', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: params.toString()
        });
        const text = await res.text();
        // Parse JSON response
        try {
            const data = JSON.parse(text);
            const userData = data.data || {};
            if (userData.Token) {
                this.setToken(userData.Token);
                this.setUsername(userData.Username);
                return userData;
            }
        } catch {}
        throw new Error(text);
    },

    async signup(username, password) {
        const params = new URLSearchParams({ username, password });
        const res = await fetch('/user/signup', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: params.toString()
        });
        const text = await res.text();
        if (text === 'SUCCESS') return { success: true };
        throw new Error(text);
    },

    // Files
    // Download by file_id (secure)
    async downloadFile(fileId) {
        const res = await this.post('/api/file/download', { file_id: fileId });
        if (res.success) return res;
        throw new Error(res.error || 'download failed');
    },
    // Download folder as ZIP
    async downloadFolder(folderId) {
        const res = await this.post('/api/file/download_zip', { folder_id: folderId });
        if (res.success) return res;
        throw new Error(res.error || 'zip failed');
    },
    listFiles(parentId, sortBy, sortOrder, limit, offset) {
        return this.post('/api/file/list', {
            parent_id: parentId || 0,
            sort_by: sortBy || 'filename',
            sort_order: sortOrder || 'asc',
            limit: limit || 50,
            offset: offset || 0
        });
    },
    createFolder(parentId, filename) {
        return this.post('/api/file/create_folder', { parent_id: parentId, filename });
    },
    renameFile(fileId, newFilename) {
        return this.post('/api/file/rename', { file_id: fileId, new_filename: newFilename });
    },
    moveFiles(fileIds, targetParentId) {
        return this.post('/api/file/move', { file_ids: fileIds, target_parent_id: targetParentId });
    },
    copyFiles(fileIds, targetParentId) {
        return this.post('/api/file/copy', { file_ids: fileIds, target_parent_id: targetParentId });
    },
    batchDelete(fileIds) {
        return this.post('/api/file/batch_delete', { file_ids: fileIds });
    },
    searchFiles(query) {
        return this.post('/api/file/search', { query });
    },
    getFileDetail(fileId) {
        return this.post('/api/file/detail', { file_id: fileId });
    },

    // Trash
    listTrash() {
        return this.post('/api/trash/list', {});
    },
    restoreTrash(fileIds) {
        return this.post('/api/trash/restore', { file_ids: fileIds });
    },
    emptyTrash() {
        return this.post('/api/trash/empty', {});
    },
    deleteForever(fileIds) {
        return this.post('/api/trash/delete_forever', { file_ids: fileIds });
    },

    // Share
    // History
    getFileHistory(fileId) {
        return this.post('/api/file/history', { file_id: fileId });
    },
    createShare(fileId, shareType, password, expireDays) {
        return this.post('/api/share/create', {
            file_id: fileId,
            share_type: shareType || 0,
            password: password || '',
            expire_days: expireDays || 7
        });
    },
    listShares() {
        return this.post('/api/share/list', {});
    },
    deleteShare(shareId) {
        return this.post('/api/share/delete', { share_id: shareId });
    },
    accessShare(token, password) {
        const params = { token };
        if (password) params.password = password;
        return this.get('/api/share/access', params);
    }
};
