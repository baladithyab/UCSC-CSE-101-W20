// Smoke test the WASM build with Node CJS require.
//
// Run as:  node smoke-test.cjs /tmp/nqueens-test.js
// Where the file at the path was compiled with -s ENVIRONMENT='web,node'.
const path = require('path')
const createModule = require(path.resolve(process.argv[2] || './embeds/nqueens.js'))

;(async () => {
  const Module = await createModule({
    print: () => {},
    printErr: () => {},
  })

  function isValidSolution(n) {
    const total = Module._nqueens_num_queens()
    if (total !== n) return false
    const positions = []
    for (let i = 0; i < total; i++) {
      positions.push([Module._nqueens_queen_row(i), Module._nqueens_queen_col(i)])
    }
    for (let i = 0; i < positions.length; i++) {
      for (let j = i + 1; j < positions.length; j++) {
        const [r1, c1] = positions[i]
        const [r2, c2] = positions[j]
        if (r1 === r2) return false
        if (c1 === c2) return false
        if (Math.abs(r1 - r2) === Math.abs(c1 - c2)) return false
      }
    }
    return true
  }

  let passed = 0, failed = 0
  for (const n of [4, 5, 6, 7, 8, 9, 10]) {
    Module._nqueens_init(n)
    const steps = Module._nqueens_solve_all()
    const done = Module._nqueens_done()
    const sol = Module._nqueens_has_solution()
    const valid = isValidSolution(n)
    if (done && sol && valid) {
      console.log(`✓ N=${n} solved in ${steps} steps; placement valid`)
      passed += 1
    } else {
      console.error(`✗ N=${n} done=${done} sol=${sol} valid=${valid}`)
      failed += 1
    }
  }

  for (const n of [3, 2]) {
    const ok = Module._nqueens_init(n)
    if (!ok) {
      console.log(`✓ N=${n} init refused (UI cap)`)
      passed += 1
    } else {
      console.error(`✗ N=${n} init was accepted; should be rejected`)
      failed += 1
    }
  }

  console.log(`\n${passed} pass / ${failed} fail`)
  process.exit(failed === 0 ? 0 : 1)
})()
