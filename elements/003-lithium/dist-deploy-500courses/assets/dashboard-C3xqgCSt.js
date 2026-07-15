import{d as e}from"./index-DmdMIfaA.js";var t=class{constructor(e,t){this.app=e,this.container=t,this.elements={}}async init(){await this.render(),this.setupEventListeners()}async render(){this.container.innerHTML=`
      <div class="dashboard-container">
        <div class="placeholder-header">
          <fa fa-chart-line></fa>
          <h2>Dashboard</h2>
        </div>
        <div class="placeholder-content">
          <p>This manager will display various dashboards and analytics.</p>
          <div class="placeholder-notice">
            <fa fa-info-circle></fa>
            <span>Dashboard module is under development.</span>
          </div>
        </div>
      </div>
    `,e(this.container)}setupEventListeners(){}teardown(){}};export{t as default};