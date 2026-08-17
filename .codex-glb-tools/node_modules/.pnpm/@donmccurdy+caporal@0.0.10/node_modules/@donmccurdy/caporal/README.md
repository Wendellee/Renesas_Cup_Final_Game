

Fork of the `@caporal/core` package, with updated dependencies and builds. For the original project and documentation, see https://caporal.io/.

## Quickstart

Installation:

```bash
npm install @donmccurdy/caporal
```

Import:

```js
// CommonJS
const { program } = require('@donmccurdy/caporal');

// ESM
import { program } from '@donmccurdy/caporal';

program
  .command(...)
  .action(...);
```
