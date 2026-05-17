const AuthView = {
    template: `
    <div class="auth-page">
      <div class="auth-card">
        <div class="logo">☁️</div>
        <h1>CloudDisk</h1>
        <p class="subtitle">{{ isSignup ? 'Create your account' : 'Sign in to your account' }}</p>

        <div class="tab-bar">
          <button :class="{ active: !isSignup }" @click="isSignup = false">Sign In</button>
          <button :class="{ active: isSignup }" @click="isSignup = true">Sign Up</button>
        </div>

        <div class="msg" :class="msgClass" :class.show="msg">{{ msg }}</div>

        <form @submit.prevent="submit">
          <div class="form-group">
            <input v-model="username" type="text" placeholder="Username" required autocomplete="username">
          </div>
          <div class="form-group">
            <input v-model="password" type="password" placeholder="Password" required autocomplete="current-password">
          </div>
          <div class="form-group" v-if="isSignup">
            <input v-model="confirm" type="password" placeholder="Confirm password" required autocomplete="new-password">
          </div>
          <button type="submit" class="btn btn-primary" :disabled="loading">
            {{ loading ? 'Processing...' : (isSignup ? 'Sign Up' : 'Sign In') }}
          </button>
        </form>
      </div>
    </div>
    `,
    data() {
        return {
            isSignup: false,
            username: '',
            password: '',
            confirm: '',
            msg: '',
            msgClass: '',
            loading: false
        };
    },
    methods: {
        async submit() {
            this.msg = '';
            this.loading = true;

            if (this.isSignup) {
                if (this.password !== this.confirm) {
                    this.msg = 'Passwords do not match.';
                    this.msgClass = 'error';
                    this.loading = false;
                    return;
                }
                try {
                    const res = await api.signup(this.username, this.password);
                    this.msg = 'Account created! You can now sign in.';
                    this.msgClass = 'success';
                    this.isSignup = false;
                } catch (e) {
                    this.msg = e.message || 'Sign up failed.';
                    this.msgClass = 'error';
                }
            } else {
                try {
                    await api.signin(this.username, this.password);
                    store.currentView = 'dashboard';
                    store.currentFolderId = 0;
                    store.currentPath = [];
                    store.refreshFiles();
                } catch (e) {
                    this.msg = e.message || 'Sign in failed.';
                    this.msgClass = 'error';
                }
            }
            this.loading = false;
        }
    }
};
