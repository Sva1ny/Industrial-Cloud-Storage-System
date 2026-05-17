// CloudDisk Vue 3 SPA
const app = Vue.createApp({
    data() {
        return { store };
    },
    computed: {
        currentViewComponent() {
            const views = {
                signin: AuthView,
                signup: AuthView,
                dashboard: DashboardView,
                trash: DashboardView,
                shares: DashboardView
            };
            return views[store.currentView] || AuthView;
        }
    },
    template: '<component :is="currentViewComponent"></component>'
});

app.component('FileToolbar', FileToolbar);
app.component('FileTable', FileTable);
app.mount('#app');
