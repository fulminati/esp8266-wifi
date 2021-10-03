
p('config');

e('click', 'connect', e => {
    o('show');
    p('connect', 'config').then(() => {
        o('hide');
        t(5000, () => r('http://<%hostname%>/welcome'));
    });
});

e('change', 'network', e => {
    if (e.value == -1) {
        o('show');
        p('scan').then(() => {
            o('hide')
        });
    }
});

e('click', 'test', e => {
    o('show');
    setTimeout(() => {
        o('hide')
    }, 3000)
});
