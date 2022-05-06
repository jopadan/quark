
import 'fs'
import 'noug/keys'
import { M } from './test'

//M(keys, 'parseFile', [ resolve('lib.keys') ])
M(keys, 'parse', [ 
`
a A
b B
c C
` 
])
M(keys, 'stringify', [ {a:'A', b:'B', c: 'C', d: [0,1,2,3,4,5,{ y:10 }]} ])
