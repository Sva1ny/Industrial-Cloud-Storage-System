// Utility functions
const utils = {
    formatSize(bytes) {
        if (!bytes || bytes === 0) return '-';
        const units = ['B', 'KB', 'MB', 'GB', 'TB'];
        const i = Math.floor(Math.log(bytes) / Math.log(1024));
        return (bytes / Math.pow(1024, i)).toFixed(i > 0 ? 1 : 0) + ' ' + units[i];
    },

    formatDate(dateStr) {
        if (!dateStr) return '-';
        const d = new Date(dateStr.replace(' ', 'T'));
        return d.toLocaleDateString('zh-CN', {
            year: 'numeric', month: '2-digit', day: '2-digit',
            hour: '2-digit', minute: '2-digit'
        });
    },

    getFileIcon(file) {
        if (file.is_folder) return '📁';
        const ext = (file.filename || '').split('.').pop().toLowerCase();
        const icons = {
            'jpg': '🖼', 'jpeg': '🖼', 'png': '🖼', 'gif': '🖼', 'webp': '🖼',
            'mp4': '🎬', 'mov': '🎬', 'avi': '🎬', 'mkv': '🎬',
            'mp3': '🎵', 'wav': '🎵', 'flac': '🎵',
            'pdf': '📄', 'doc': '📝', 'docx': '📝', 'xls': '📊', 'xlsx': '📊',
            'ppt': '📽', 'pptx': '📽',
            'zip': '🗜', 'rar': '🗜', '7z': '🗜', 'gz': '🗜',
            'txt': '📃', 'html': '🌐', 'js': '⚙', 'json': '⚙',
        };
        return icons[ext] || '📄';
    },

    escapeHtml(str) {
        const div = document.createElement('div');
        div.textContent = str;
        return div.innerHTML;
    },

    debounce(fn, delay) {
        let timer;
        return function (...args) {
            clearTimeout(timer);
            timer = setTimeout(() => fn.apply(this, args), delay);
        };
    }
};
