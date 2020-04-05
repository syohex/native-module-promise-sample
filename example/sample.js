const native_promise = require('../index')

async function main() {
  let v = await native_promise.add(1, 2)
  console.log(`v = ${v}`)

  v = await native_promise.sub(10, 3)
  console.log(`v = ${v}`)

  v = await native_promise.mul(16, 15)
  console.log(`v = ${v}`)

  v = await native_promise.div(999, 9)
  console.log(`v = ${v}`)

  try {
    v = await native_promise.div(1, 0)
  } catch (err) {
    console.log(`Error: ${err}`)
  }
}

main().catch((err) => {
  console.log(err)
  process.exit(1)
})
