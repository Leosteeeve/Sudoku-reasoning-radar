import assert from 'node:assert/strict';
import test from 'node:test';

import { unavailableMessage } from './not-yet-available.mjs';

test('placeholder command explains that its application is not available', () => {
  assert.equal(
    unavailableMessage('dev:web'),
    'dev:web is not yet available: the v1 Web application has not been created.',
  );
});
