const { VrpcRemote } = require('vrpc')

const vrpcRemote = new VrpcRemote({
  agent: '<BOARD-MAC-ADDRESS>',
  domain: 'vrpc',
  broker: 'mqtts://vrpc.io:8883'
})

async function main () {
  await vrpcRemote.callStatic({
    className: '__global__',
    functionName: 'ledOn'
  })
  await new Promise(resolve => setTimeout(resolve, 2000))
  await vrpcRemote.callStatic({
    className: '__global__',
    functionName: 'ledOff'
  })
  await vrpcRemote.end()
}

main().catch(err => console.log(`An error happened: ${err.message}`))
