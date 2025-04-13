
import bindings from 'bindings';
const addon = bindings('awe-nng');


class App {
  prev = Date.now();
  stamp = 0;

  periodic() {
    setInterval(() => {
      const duration = Date.now() - this.prev;
      this.prev = Date.now();
      this.stamp = this.stamp + 1;
      console.log(`Iteration ${this.stamp} with duration ${duration} sec.`);
    }, 100);
  }

  async query() {
    const req = new addon.Socket('req');
    await req.Dial('tcp://127.0.0.1:5555');
    await req.Send(JSON.stringify({
        title: 'Главная страница',
        format: 'application/xml',
    }));
    const msg = await req.Recv();
    console.log(`ответ от сервера: msg = ${msg}`);
    req.Close();
  }

  run() {
    this.periodic();
    setTimeout(() => {
        this.query()
          .then(() => console.log('Done.'))
          .catch((err) => console.error(err));
    }, 1000);
  }
  
}

const app = new App();
app.run();
