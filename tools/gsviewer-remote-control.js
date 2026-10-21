import * as fs from 'node:fs/promises';
import * as net from 'node:net';
import * as rl from 'node:readline/promises';

const argv = process.argv.slice(2);

// const socketPath = String.raw`\\.\pipe\gsvar\cmd2.sock`;
const socketPath = 61419;

if (argv.join('').trim() == '-') {
	
	const socket = net.createConnection(socketPath);
	
	socket.pipe(process.stdout);
	
	process.stdin.pipe(socket);	
	
} else if (argv.join(' ').trim().length > 0) {
	
	const socket = net.createConnection(socketPath);
	
	socket.pipe(process.stdout);
	
	socket.end(argv.join(' ') + '\n');
	
} else {
	
	const input = rl.createInterface({ input: process.stdin, output: process.stderr });

	while (true)
		try {
			const data = await input.question(">  ");

			const socket = net.createConnection(socketPath);
			
			if (!socket.write(data.trim() + '\n'))
				await new Promise((Ok, Err) => socket
					.once('drain', () => Ok())
					.once('error', err => Err(err)));
					
			socket.destroySoon();
		} catch (err) {
			console.error(err);
			break;
		}
		
}