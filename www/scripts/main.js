window.onload = () => {
    'use strict';
    if ('serviceWorker' in navigator) {
        navigator.serviceWorker.register('scripts/sw.js')
            .then(reg => console.log(reg))
            .catch(err => console.log(err));
    }
  }