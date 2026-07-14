import { fileURLToPath } from 'node:url';
import path from 'node:path';

const subjects = {
  'dev:web': 'the v1 Web application has not been created',
  'dev:desktop': 'the v1 desktop application has not been created',
  build: 'the v1 Web and desktop applications have not been created',
  'package:windows': 'the v1 Windows application has not been created',
};

export function unavailableMessage(command) {
  const subject = subjects[command] ?? 'this application has not been created';
  return `${command} is not yet available: ${subject}.`;
}

const isMain = process.argv[1]
  && path.resolve(process.argv[1]) === path.resolve(fileURLToPath(import.meta.url));

if (isMain) {
  const command = process.argv[2] ?? 'command';
  console.error(unavailableMessage(command));
  process.exitCode = 1;
}
