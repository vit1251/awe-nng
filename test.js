
import { Socket } from './index.js';

class App {
  prev = Date.now();
  stamp = 0;
  reqId = 0;

  periodic() {
    setInterval(() => {
      const duration = Date.now() - this.prev;
      this.prev = Date.now();
      this.stamp = this.stamp + 1;
      console.log(`Iteration ${this.stamp} with duration ${duration} sec.`);
    }, 100);
  }

  async query() {
    /* Step 1. Update request number */
    this.reqId = this.reqId + 1;
    /* Step 2. Perform request */
    const req = new Socket('req');
    await req.Dial('tcp://127.0.0.1:5555');
    console.log(`Request ${this.reqId}...`);
    await req.Send(JSON.stringify({
        title: 'Welcome',
        format: 'application/xml',
    }));
    const msg = await req.Recv();
    console.log(`Response ${this.reqId}: msg = ${msg}`);
    req.Close();
  }

  registerRequest(timeout = 1_000) {
    setTimeout(() => {
        this.query()
          .then(() => {
              console.log('Done.')
              this.registerRequest(timeout);
          })
          .catch((err) => console.error(err));
    }, timeout);
  }

  run() {
    this.periodic();
    this.registerRequest(1_200);
  }

}

const app = new App();
app.run();
