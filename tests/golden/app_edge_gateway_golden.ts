type EdgeGatewayProperties = { [key: string]: string };

const states = {
    created: 1,
    pending_access_delivery: 2,
    pending_child_install: 3,
    pending_gateway_proof: 4,
    active: 5,
    expired: 6,
    cancelled: 7,
    rollback_required: 8,
    revoking: 9,
    detached: 10,
} as const;

function parse(text: string): EdgeGatewayProperties {
    const output: EdgeGatewayProperties = Object.create(null) as EdgeGatewayProperties;
    text.split(/\r?\n/).forEach(line => {
        if (!line || line.charAt(0) === '#') return;
        const split = line.indexOf('=');
        if (split <= 0) throw new Error('invalid fixture');
        output[line.slice(0, split)] = line.slice(split + 1);
    });
    return output;
}

const values = parse(require('fs').readFileSync(process.argv[2], 'utf8'));
Object.entries(states).forEach(([name, code]) => {
    if (Number(values[`topology_${name}`]) !== code) {
        throw new Error(`App topology state drifted: ${name}`);
    }
});
console.log('App Edge Gateway golden passed: topology states');
