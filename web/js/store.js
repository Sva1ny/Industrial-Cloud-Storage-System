// Reactive state store
const store = Vue.reactive({
    currentView: 'signin',  // 'signin' | 'signup' | 'dashboard' | 'trash' | 'shares'
    currentFolderId: 0,
    currentPath: [],        // [{id, name}]
    files: [],
    selectedFiles: new Set(),
    sortBy: 'filename',
    sortOrder: 'asc',
    viewMode: 'list',       // 'list' | 'grid'
    searchQuery: '',
    isLoading: false,
    trashItems: [],
    shares: [],

    // Transfer queue
    transfers: [],           // [{id, name, type, progress, status}]

    async navigateToFolder(folderId, folderName) {
        this.currentFolderId = folderId;
        if (folderId === 0) {
            this.currentPath = [];
        } else if (folderName) {
            this.currentPath.push({ id: folderId, name: folderName });
        }
        this.selectedFiles.clear();
        await this.refreshFiles();
    },

    navigateToPath(index) {
        if (index === -1) {
            this.currentFolderId = 0;
            this.currentPath = [];
        } else {
            const target = this.currentPath[index];
            this.currentFolderId = target.id;
            this.currentPath = this.currentPath.slice(0, index);
        }
        this.selectedFiles.clear();
        this.refreshFiles();
    },

    async refreshFiles() {
        this.isLoading = true;
        try {
            const res = await api.listFiles(this.currentFolderId, this.sortBy, this.sortOrder);
            if (res.success) this.files = res.data.files || [];
        } catch (e) { console.error('refresh error:', e); }
        this.isLoading = false;
    },

    async refreshTrash() {
        const res = await api.listTrash();
        if (res.success) this.trashItems = res.data.items || [];
    },

    async refreshShares() {
        const res = await api.listShares();
        if (res.success) this.shares = res.data.shares || [];
    },

    toggleSelect(fileId) {
        if (this.selectedFiles.has(fileId))
            this.selectedFiles.delete(fileId);
        else
            this.selectedFiles.add(fileId);
    },

    selectAll() {
        if (this.selectedFiles.size === this.files.length)
            this.selectedFiles.clear();
        else
            this.files.forEach(f => this.selectedFiles.add(f.id));
    },

    // Transfer queue
    addTransfer(name, type) {
        const id = Date.now() + Math.random();
        this.transfers.unshift({ id, name, type, progress: 0, status: 'active' });
        return id;
    },
    updateTransfer(id, progress) {
        const t = this.transfers.find(t => t.id === id);
        if (t) t.progress = progress;
    },
    completeTransfer(id, status) {
        const t = this.transfers.find(t => t.id === id);
        if (t) t.status = status || 'done';
    },
    removeTransfer(id) {
        this.transfers = this.transfers.filter(t => t.id !== id);
    }
});
