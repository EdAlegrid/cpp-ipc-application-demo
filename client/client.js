const { Client } = require('m2m');

let client = new Client();

client.connect(() => {

  client.watch({id:300, channel:'ipc-channel'}, (data) => {  
    try{
      let jd = JSON.parse(data);
      console.log('rcvd json data:', jd);
    }
    catch (e){
      console.log('rcvd string data:', data);
    }
  });

});
